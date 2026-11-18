#!/usr/bin/env python3
"""Normalize exact RSP reciprocal inputs with CLZ instead of a shifting loop."""
from pathlib import Path
import sys
root=Path(sys.argv[1]).resolve();project=Path(__file__).resolve().parents[1]
if root==project/'vendor/parallel-n64':raise SystemExit('Use a build copy')
p=root/'mupen64plus-video-angrylion/rdp_emit_rsp.c';s=p.read_text()
marker='/* NPS64 exact reciprocal normalization.'
if marker not in s:
    names=['rsp_div_core','rsp_rcp32','rsp_rcp32_dp','rsp_rcp16','rsp_rsq32']
    refs=[]
    for name in names:
        a=s.index(('static ' if name=='rsp_div_core' else '')+'int32_t '+name+'(')
        opening=s.index('{',a);b=opening+1;depth=1
        while depth:
            depth+=(s[b]=='{')-(s[b]=='}');b+=1
        refs.append(s[a:b])
    old='''            for (shift = 0; (ua & 0x80000000u) == 0u; ua <<= 1, shift++)
                ;'''
    assert s.count(old)==3
    s=s.replace(old,'''            /* NPS64 exact reciprocal normalization. Nonzero is checked above. */
#if defined(__GNUC__) || defined(__clang__)
            shift = __builtin_clz(ua);
            ua <<= shift;
#else
'''+old+'''
#endif''')
    reference='\n'.join(refs)
    # Longer names first so rsp_rcp32 does not rewrite rsp_rcp32_dp twice.
    import re
    for name in sorted(names,key=len,reverse=True):
        reference=re.sub(r'\b'+name+r'\b','nps64_reference_'+name,reference)
    s+='\n#ifdef NPS64_RSP_DIV_ORACLE\n'+reference+'\n#endif\n';p.write_text(s)
print('Exact reciprocal normalization prepared in',root)
