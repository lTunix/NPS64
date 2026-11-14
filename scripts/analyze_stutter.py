#!/usr/bin/env python3
"""Summarize 00.28+ cadence runs without replacing tail latency with mean FPS."""
import argparse,json,re
from pathlib import Path

def analyze(text):
    segments=[];current=None;rom=None
    def begin():return {'rom':rom,'measurement':'present_return_intervals','events':[],'last':{},'complete':False}
    def finish():
        nonlocal current
        if current and (current['last'] or current['events']):segments.append(current)
        current=None
    for line in text.splitlines():
        if '[load] path=' in line:
            finish();rom=line.split('name=',1)[-1] if 'name=' in line else line.split('[load] path=',1)[-1]
        if '[stutter]' in line:
            fields=dict(re.findall(r'(\w+)=([^\s]+)',line.split('[stutter]',1)[1]))
            if not {'end_s','duration_ms','frames','max_ms','excess_ms'}<=fields.keys():continue
            if current is None:current=begin()
            current['events'].append({k:float(fields[k]) for k in ['end_s','duration_ms','frames','max_ms','excess_ms']})
        if '[cadence]' in line:
            f=dict(re.findall(r'(\w+)=([^\s]+)',line.split('[cadence]',1)[1]))
            if not {'frames','slow','observed_ms','longest_ms','episodes','active_ms'}<=f.keys():continue
            if current and current['last'] and int(f['frames'])<int(current['last']['frames']):finish()
            if current is None:current=begin()
            current['last']=f
            if f.get('phase') in ['play_end','shutdown']:current['complete']=True;finish()
    finish();result=[]
    for segment in segments:
        s=segment['last'];row={'rom':segment['rom'],'measurement':segment['measurement'],'complete_segment':segment['complete']}
        for key in ['frames','slow','slow_pct','observed_ms','slow_ms','excess_ms','max_ms','episodes','longest_frames','longest_ms','active_frames','active_ms','over_1_5x','over_2x','over50','over100']:
            if key in s:row[key]=float(s[key])
        observed=row.get('observed_ms',0)
        if observed>0:
            row['slow_time_pct']=100*row.get('slow_ms',0)/observed
            row['hitches_1_5x_per_minute']=60000*row.get('over_1_5x',0)/observed
            row['over50_per_minute']=60000*row.get('over50',0)/observed
            row['over100_per_minute']=60000*row.get('over100',0)/observed
        row['reported_hitch_events']=len(segment['events'])
        row['longest_events']=sorted(segment['events'],key=lambda event:event['duration_ms'],reverse=True)[:10]
        result.append(row)
    return {'segments':result,'notes':['Counters are cumulative within an uninterrupted play segment.','Events include loading; use end_s to distinguish warmup from later gameplay.','Intervals measure presentation returns, not physical display timestamps.','Event detail starts at 1.5 target periods; episode totals also count smaller sustained slowdowns.']}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('log',type=Path);parser.add_argument('--last-run',action='store_true');args=parser.parse_args()
    text=args.log.read_text(errors='replace')
    if args.last_run:
        runs=list(re.finditer(r'NPS64 Vulkan Game Test [0-9.]+;',text))
        if runs:text=text[runs[-1].start():]
    print(json.dumps(analyze(text),indent=2,ensure_ascii=False))
