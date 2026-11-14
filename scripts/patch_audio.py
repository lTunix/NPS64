"""Correct per-field audio padding without changing AI interrupt timing."""
from pathlib import Path

def patch_audio(root):
    backend = root/'mupen64plus-core/src/plugin/audio_libretro/audio_backend_libretro.c'
    text = backend.read_text()
    text = '#include "audio_plugin.h"\n' + text
    old = 'static double   current_sample_rate_exact;'
    assert text.count(old)==1
    text = text.replace(old, '''/* Count every DMA/poll delivery during this field, including emergency
 * flushes. A DMA completion already delivered audio even when the last
 * ai_hand_over_played() call at the VI boundary returns zero. */
static size_t nps64_field_frames;
static uint64_t nps64_field_phase;
static int64_t nps64_field_credit;
static unsigned nps64_field_divider;
''' + old)
    old = '   current_sample_rate = 0;'
    assert text.count(old)==2
    text = text.replace(old, old+'''
   current_sample_rate_exact = 0.0;
   nps64_field_frames = 0;
   nps64_field_phase = 0;
   nps64_field_credit = 0;
   nps64_field_divider = 0;''')
    old = '   emit_frames(audio_acc_frames);\n}'
    assert text.count(old)==1
    text = text.replace(old, '''   emit_frames(audio_acc_frames);
   nps64_field_frames = 0;
}

/* Balance the DAC budget across fields: AI_LEN is aligned to two stereo
 * frames, so a short field can be followed by a longer one. Padding every
 * individual deficit inserted zeros into otherwise continuous DMA audio.
 * Allow two frames of delivery quantization; preserve excess as credit.
 * A real idle field still supplies silence to the frontend clock. */
void nps64_audio_pad_field(unsigned ticks, unsigned divider)
{
   uint64_t total;
   size_t want;
   if (!ticks || !divider)
      return;
   if (divider != nps64_field_divider)
   {
      nps64_field_phase = 0;
      nps64_field_credit = 0;
      nps64_field_divider = divider;
   }
   total = nps64_field_phase + ticks;
   want = (size_t)(total / divider);
   nps64_field_phase = total % divider;
   nps64_field_credit += (int64_t)nps64_field_frames - (int64_t)want;
   if (nps64_field_credit < 0 &&
       (nps64_field_credit < -2 || nps64_field_frames == 0))
   {
      size_t missing = (size_t)(-nps64_field_credit);
      push_audio_silence_via_libretro(missing);
      nps64_field_credit = 0;
   }
}''')
    old = '   frames = size >> 2;                   /* 4 bytes per stereo frame */'
    assert text.count(old)==1
    text = text.replace(old, old+'\n   nps64_field_frames += frames; /* include emergency drains */')
    old = 'void push_audio_silence_via_libretro(size_t frames)\n{'
    assert text.count(old)==1
    text = text.replace(old, old+'\n   nps64_field_frames += frames;')
    backend.write_text(text)
    header = backend.with_name('audio_plugin.h')
    text = header.read_text()
    text = text.replace('void push_audio_silence_via_libretro(size_t frames);',
                        'void push_audio_silence_via_libretro(size_t frames);\nvoid nps64_audio_pad_field(unsigned ticks, unsigned divider);')
    header.write_text(text)
    ai = root/'mupen64plus-core/src/device/rcp/ai/ai_controller.c'
    text = ai.read_text()
    start = text.index('void ai_deliver_frame(struct ai_controller* ai)')
    # This is the last function in the pinned source; avoid editing DMA timing.
    assert text[start:].rstrip().endswith('}')
    text = text[:start]+'''void ai_deliver_frame(struct ai_controller* ai)
{
    unsigned int divider = 1 + ai->regs[AI_DACRATE_REG];
    if (ai->fifo[0].duration != 0)
        ai_hand_over_played(ai, get_remaining_dma_length(ai));
    if (ai->regs[AI_DACRATE_REG] != 0)
        nps64_audio_pad_field(ai->vi->delay, divider);
}
'''
    ai.write_text(text)
