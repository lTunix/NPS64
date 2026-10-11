"""Apply a conservative SURFACE_SYNC diagnostic handler to shadPS4 0.19.0.

The modified emulator remains under ignored build/. This is not a hardware
cache model or a production performance implementation.
"""
from pathlib import Path
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('source', type=Path)
args = parser.parse_args()
path = args.source / 'src/video_core/amdgpu/liverpool.cpp'
text = path.read_text()
marker = 'NPS64 local SURFACE_SYNC diagnostic'
if marker in text:
    print('Patch already applied')
else:
    include = '#include "video_core/renderer_vulkan/vk_rasterizer.h"'
    assert text.count(include) == 1
    text = text.replace(include, include + '\n#include "video_core/renderer_vulkan/vk_runtime.h"')
    anchor = '            case PM4ItOpcode::AcquireMem: {'
    assert text.count(anchor) == 1, 'Review upstream packet handlers before patching'
    handler = '''            case PM4ItOpcode::SurfaceSync: {
                // NPS64 local SURFACE_SYNC diagnostic: conservative global scope.
                // GCN7 payload is coher_cntl, size, base, poll_interval (four words).
                // No cache action is discarded based on an unsupported bitfield.
                ASSERT_MSG(count == 4, "Unexpected SURFACE_SYNC payload size {}", count);
                if (rasterizer) {
                    auto& scheduler = rasterizer->GetScheduler();
                    scheduler.EndRendering();
                    rasterizer->GetRuntime().FlushBarriers();
                    const vk::MemoryBarrier2 barrier{
                        .srcStageMask = vk::PipelineStageFlagBits2::eAllCommands |
                                        vk::PipelineStageFlagBits2::eHost,
                        .srcAccessMask = vk::AccessFlagBits2::eMemoryRead |
                                         vk::AccessFlagBits2::eMemoryWrite,
                        .dstStageMask = vk::PipelineStageFlagBits2::eAllCommands |
                                        vk::PipelineStageFlagBits2::eHost,
                        .dstAccessMask = vk::AccessFlagBits2::eMemoryRead |
                                         vk::AccessFlagBits2::eMemoryWrite,
                    };
                    const vk::DependencyInfo dependency{
                        .memoryBarrierCount = 1,
                        .pMemoryBarriers = &barrier,
                    };
                    scheduler.CommandBuffer().pipelineBarrier2(dependency);
                    // Submit the barrier and wait for real host GPU completion.
                    rasterizer->Finish();
                    LOG_DEBUG(Render, "NPS64 SURFACE_SYNC global barrier completed");
                }
                break;
            }
'''
    path.write_text(text.replace(anchor, handler + anchor))
    print('Applied conservative global GPU barrier and completion wait')

# Sony compiler extent NOPs are optional metadata, not mandatory PM4 packets.
# Keep shadPS4's existing extent inference when Mesa supplies no such hint.
text = path.read_text()
old = '''                    if (nop_offset == 0x04) {
                        ASSERT_MSG(payload[nop_offset] == 0xc0001000,
                                   "NOP hint is missing in CB setup sequence");
                        last_cb_extent[col_buf_id].raw = payload[nop_offset + 1];
                    }'''
new = '''                    if (nop_offset == 0x04 && dcb.size() > nop_offset + 3 &&
                        payload[nop_offset] == 0xc0001000) {
                        last_cb_extent[col_buf_id].raw = payload[nop_offset + 1];
                    } else {
                        last_cb_extent[col_buf_id].raw = 0; // Mesa: infer extent without NOP.
                    }'''
if old in text:
    assert text.count(old) == 1
    text = text.replace(old, new)
else:
    assert new in text, 'Review CB extent handling before patching'
old = '''                    if (header->type3.count == 8) {
                        ASSERT_MSG(payload[20] == 0xc0001000,
                                   "NOP hint is missing in DB setup sequence");
                        last_db_extent.raw = payload[21];'''
new = '''                    if (header->type3.count == 8 && dcb.size() > 23 &&
                        payload[20] == 0xc0001000) {
                        last_db_extent.raw = payload[21];'''
if old in text:
    assert text.count(old) == 1
    text = text.replace(old, new)
else:
    assert new in text, 'Review DB extent handling before patching'
path.write_text(text)
print('Optional CB/DB extent hints bounded; missing hints use existing inference')

# SSA repair/rewrites can expose constant class masks after the earlier pass.
# Resolve them before hardware intrinsic lowering requires an immediate mask.
path = args.source / 'src/shader_recompiler/recompiler.cpp'
text = path.read_text()
anchor = '    Shader::Optimization::LowerHardwareIntrinsics(program);'
replacement = '''    // NPS64: resolve constants exposed by structured SSA before intrinsic lowering.
    Shader::Optimization::ConstantPropagationPass(program.post_order_blocks);
''' + anchor
if replacement not in text:
    assert text.count(anchor) == 1
    path.write_text(text.replace(anchor, replacement))
print('Structured SSA constants propagated before hardware intrinsic lowering')

# Mesa may supply a dynamic V_CMP_CLASS mask. Classify IEEE-754 bits exactly,
# including signed zero, denormals, infinities and quiet/signaling NaNs.
path = args.source / 'src/shader_recompiler/ir/passes/lower_hardware_intrinsics.cpp'
text = path.read_text()
start = text.index('static void LowerCmpClass(')
end = text.index('\nstatic void LowerPackedAncillary', start)
replacement = '''static void LowerCmpClass(IR::Block& block, IR::Inst& inst) {
    // NPS64 local dynamic IEEE-754 class mask support.
    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    const IR::U32 bits = ir.BitCast<IR::U32>(IR::F32{inst.Arg(0)});
    const IR::U32 mask{inst.Arg(1)};
    const auto zero = ir.Imm32(0);
    const auto negative = ir.INotEqual(ir.BitwiseAnd(bits, ir.Imm32(0x80000000)), zero);
    const auto exponent = ir.BitwiseAnd(bits, ir.Imm32(0x7f800000));
    const auto mantissa = ir.BitwiseAnd(bits, ir.Imm32(0x007fffff));
    const auto exponent_zero = ir.IEqual(exponent, zero);
    const auto exponent_full = ir.IEqual(exponent, ir.Imm32(0x7f800000));
    const auto mantissa_zero = ir.IEqual(mantissa, zero);
    const auto quiet = ir.INotEqual(ir.BitwiseAnd(mantissa, ir.Imm32(0x00400000)), zero);
    const IR::U32 normal{ir.Select(negative, ir.Imm32(1u << 3), ir.Imm32(1u << 8))};
    const IR::U32 denorm{ir.Select(negative, ir.Imm32(1u << 4), ir.Imm32(1u << 7))};
    const IR::U32 signed_zero{ir.Select(negative, ir.Imm32(1u << 5), ir.Imm32(1u << 6))};
    const IR::U32 infinity{ir.Select(negative, ir.Imm32(1u << 2), ir.Imm32(1u << 9))};
    const IR::U32 nan{ir.Select(quiet, ir.Imm32(1u << 1), ir.Imm32(1u << 0))};
    const IR::U32 small{ir.Select(mantissa_zero, signed_zero, denorm)};
    const IR::U32 special{ir.Select(mantissa_zero, infinity, nan)};
    const IR::U32 finite{ir.Select(exponent_zero, small, normal)};
    const IR::U32 category{ir.Select(exponent_full, special, finite)};
    inst.ReplaceUsesWithAndRemove(ir.INotEqual(ir.BitwiseAnd(mask, category), zero));
}
'''
text = text[:start] + replacement + text[end:]
path.write_text(text)
print('Dynamic IEEE-754 class masks lowered without dropping comparisons')
