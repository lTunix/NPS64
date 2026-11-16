#!/usr/bin/env python3
"""Emit fully visible GSP triangles without temporary polygon/vertex copies."""
from pathlib import Path
import sys
root=Path(sys.argv[1]).resolve()
project=Path(__file__).resolve().parents[1]
if root==project/'vendor/parallel-n64':raise SystemExit('Use a build copy')
p=root/'mupen64plus-video-angrylion/rdp_emit_frontend.c';s=p.read_text()
marker='/* NPS64 direct emission for triangles wholly inside the clip planes. */'
if marker not in s:
 start=s.index('int gsp_triangle(GSPState *s, int32_t *cmd, int i0, int i1, int i2,')
 opening=s.index('{',start);depth=1;end=opening+1
 while depth:
  depth+=(s[end]=='{')-(s[end]=='}');end+=1
 original=s[start:end]
 a=original.index('            v0.cx = tv[0].cx;')
 b=original.index('            if (nc > 0)',a)
 conv=original[a:b]
 for i,n in enumerate(['a','b','c']):conv=conv.replace('tv['+str(i)+'].',n+'->')
 conv=conv.replace('nc = bridge_add_triangle(cmd + total,','nc = bridge_add_triangle(cmd,')
 helper=marker+'''
static int nps64_gsp_emit_triangle(const GSPState *s,int32_t *cmd,
                                  const GSPVertex *a,const GSPVertex *b,const GSPVertex *c,
                                  int textured,int z_buffered)
{
    BridgeVertex v0,v1,v2;int nc;
'''+conv+'''    return nc>0?nc:0;
}
'''
 patched=original
 patched=patched.replace('            BridgeVertex v0, v1, v2;\n','')
 # Move the identical conversion into one helper for both fan and direct paths.
 aa=patched.index('            v0.cx = tv[0].cx;');bb=patched.index('            if (nc > 0)',aa)
 patched=patched[:aa]+'''            nc=nps64_gsp_emit_triangle(s,cmd+total,&tv[0],&tv[1],&tv[2],textured,z_buffered);
'''+patched[bb:]
 anchor='    if (s->clip_fan_first == 2)\n'
 assert patched.count(anchor)>=1
 patched=patched.replace(anchor,'''    /* Unclipped triangles keep the original a,b,c order under both
     * normal fan conventions. Preserve the clipper for every boundary,
     * rejection microcode, Wipeout and Rogue Squadron special case. */
    if (!s->rs_clip_model && s->clip_fan_first != 2 && !s->clip_reject
        && !((oa|ob|oc) & ((1u<<20)|(1u<<21)|(1u<<28)|(1u<<29)
                          |(1u<<14)|(1u<<7)|(s->clip_near_z?(1u<<6):0u))))
        return nps64_gsp_emit_triangle(s,cmd,a,b,c,textured,z_buffered);

'''+anchor,1)
 oracle=original.replace('int gsp_triangle(', 'int nps64_reference_gsp_triangle(',1)
 s=s[:start]+helper+patched+'\n#ifdef NPS64_GSP_TRIANGLE_ORACLE\n'+oracle+'\n#endif\n'+s[end:]
 p.write_text(s)
print('GSP direct visible-triangle emission prepared in',root)
