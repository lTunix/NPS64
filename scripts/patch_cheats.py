#!/usr/bin/env python3
"""Bound the libretro cheat parser; disable without discarding restore values."""
from pathlib import Path
import sys
FUNCTION=r'''/* NPS64 bounded GameShark parser. Parse all input before changing state. */
void retro_cheat_set(unsigned index, bool enabled, const char *text)
{
    char name[32];
    m64p_cheat_code codes[128];
    unsigned count = 0;
    snprintf(name, sizeof(name), "cheat_%u", index);
    if (!enabled) { cheat_set_enabled(&g_cheat_ctx, name, 0); return; }
    if (!text) return;
    const char *p = text;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' || *p == '+') p++;
        if (!*p) break;
        if (count == 128) return;
        unsigned long values[2] = {0, 0};
        for (unsigned part = 0; part < 2; part++) {
            unsigned digits = 0;
            while ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')) {
                unsigned v = *p <= '9' ? (unsigned)(*p-'0') : *p <= 'F' ? (unsigned)(*p-'A'+10) : (unsigned)(*p-'a'+10);
                if (++digits > (part ? 4u : 8u)) return;
                values[part] = (values[part]<<4) | v; p++;
            }
            if (digits != (part ? 4u : 8u)) return;
            if (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && *p != '+') return;
            while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' || *p == '+') p++;
        }
        codes[count].address = (uint32_t)values[0];
        codes[count].value = (int)values[1]; count++;
    }
    if (count && cheat_add_new(&g_cheat_ctx, name, codes, (int)count)) cheat_set_enabled(&g_cheat_ctx, name, 1);
}
void nps64_cheat_button(int active) { extern void event_set_gameshark(int); event_set_gameshark(active); }
'''
def patch(root):
 p=Path(root)/'libretro/libretro.c';s=p.read_text();a=s.index('void retro_cheat_set(');b=s.index('\nvoid vbo_disable',a)
 if '/* NPS64 bounded GameShark parser.' in s[:a]:a=s.rindex('/* NPS64 bounded GameShark parser.',0,a)
 s=s[:a]+FUNCTION+'\n'+s[b:]
 if p.read_text()!=s:p.write_text(s)
 p=Path(root)/'mupen64plus-core/src/main/cheat.c';s=p.read_text()
 marker='static int execute_cheat(struct r4300_core* r4300, uint32_t address, uint32_t value, uint32_t* old_value)\n{'
 guard='\n    /* NPS64: reject RAM access outside the actual expansion size. */\n    uint32_t kind = address >> 24, offset = address & 0xffffff;\n    if (kind != 0xee && (offset >= r4300->rdram->dram_size ||\n        ((kind & 1) && ((offset & 1) || r4300->rdram->dram_size - offset < 2)))) return 0;\n'
 if 'NPS64: reject RAM access' not in s:
  assert marker in s;s=s.replace(marker,marker+guard)
 s=s.replace('cheat_code_t *code = malloc(sizeof(*code));\n', 'cheat_code_t *code = malloc(sizeof(*code));\n                if (!code) { cheat->enabled = 0; return 0; }\n') if 'if (!code) { cheat->enabled' not in s else s
 if p.read_text()!=s:p.write_text(s)
if __name__=='__main__':patch(sys.argv[1])
