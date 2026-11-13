#!/usr/bin/env python3
"""Prepare the split renderer's general pipeline before asynchronous specialization."""
from pathlib import Path
import sys
root=Path(sys.argv[1]).resolve();project=Path(__file__).resolve().parents[1]
if root==project/'vendor/parallel-n64':raise SystemExit('Use a build copy, not vendor')
p=root/'mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp/rdp_renderer.cpp';s=p.read_text()
if '[pipeline-hybrid] generic ready' not in s:
 old='''void Renderer::set_shader_bank(const ShaderBank *bank)
{
\tshader_bank = bank;
}''';assert s.count(old)==1
 new='''void Renderer::set_shader_bank(const ShaderBank *bank)
{
    shader_bank = bank;
    if (caps.force_sync || caps.ubershader)return;
    const uint32_t scales[2]={0u, uint32_t(trailing_zeroes(caps.upscaling)) << RASTERIZATION_UPSCALING_LOG2_BIT_OFFSET};
    const unsigned variants=caps.upscaling>1?2u:1u;
    for (unsigned i=0;i<variants;i++)
    {
        // Finite general shader, independent of game history. No dispatch or
        // ROM-specific recipe load: only create the pipeline used by fallback.
        Nps64GpuRecipe recipe={};recipe.mask=7;
        recipe.shader=shader_bank->rasterizer->get_shader(Vulkan::ShaderStage::Compute)->get_hash();
        recipe.constants[0]=ImplementationConstants::TileWidth;
        recipe.constants[1]=ImplementationConstants::TileHeight;
        recipe.constants[2]=scales[i];
        Util::Hasher hash;hash.u64(shader_bank->rasterizer->get_hash());
        const auto &layout=shader_bank->rasterizer->get_pipeline_layout()->get_resource_layout();
        recipe.mask &= layout.combined_spec_constant_mask;
        hash.u32(recipe.mask);
        for (unsigned k=0;k<NPS64_RECIPE_CONSTANTS;k++)if (recipe.mask&(1u<<k))hash.u32(recipe.constants[k]);
        hash.s32(0);recipe.pipeline=hash.get();
        bool ready=Vulkan::CommandBuffer::nps64_prewarm_recipe(device,shader_bank->rasterizer,VK_NULL_HANDLE,recipe);
        LOGI("[pipeline-hybrid] generic ready=%u hash=%016llx scale=%u\\n",unsigned(ready),
             static_cast<unsigned long long>(recipe.pipeline),scales[i]);
        if (!ready){caps.force_sync=true;LOGE("[pipeline-hybrid] generic unavailable; specialization stays synchronous\\n");break;}
    }
}'''
 s=s.replace(old,new)
 anchor='\t\t\t\trdp_worker_push(pipeline_worker->worker, node);';assert s.count(anchor)==1
 s=s.replace(anchor,anchor+'''
                LOGI("[pipeline-async] enqueue specialized=%016llx general_fallback=1\\n",
                     static_cast<unsigned long long>(node->compile.hash));''')
 # The worker owns node after push; log before handing it over.
 s=s.replace('''\t\t\t\trdp_worker_push(pipeline_worker->worker, node);
                LOGI("[pipeline-async] enqueue specialized=%016llx general_fallback=1\\n",
                     static_cast<unsigned long long>(node->compile.hash));''','''                LOGI("[pipeline-async] enqueue specialized=%016llx general_fallback=1\\n",
                     static_cast<unsigned long long>(node->compile.hash));
\t\t\t\trdp_worker_push(pipeline_worker->worker, node);''')
 p.write_text(s)
print('Hybrid split shader preparation ready in '+str(root))
# NOSCHED disables RADV's binary cache. Once queued, checking the live
# program map is sufficient; never ask the render thread to create/probe a
# specialized pipeline while the background compiler owns that work.
s=p.read_text()
if 'NPS64 nonblocking program lookup' not in s:
 old='''\t\tif (!caps.force_sync && !cmd.flush_pipeline_state_without_blocking())
\t\t{
\t\t\tVulkan::DeferredPipelineCompile compile;
\t\t\tcmd.extract_pipeline_state(compile);
\t\t\tif (pending_async_pipelines.count(compile.hash) == 0)'''
 new='''        // NPS64 nonblocking program lookup: known handles or general shader.
        Vulkan::DeferredPipelineCompile compile;
        if (!caps.force_sync)cmd.extract_pipeline_state(compile);
        if (!caps.force_sync && !compile.program->get_pipeline(compile.hash).pipeline)
        {
\t\t\tif (pending_async_pipelines.count(compile.hash) == 0)'''
 assert s.count(old)==1
 p.write_text(s.replace(old,new))
