import sys
sys.path.insert(0,'scripts')
from analyze_stutter import analyze
text='''[load] path=/games/A.z64 name=A
[stutter] end_s=2 duration_ms=1000 frames=50 max_ms=20 excess_ms=167
[cadence] phase=interval frames=60 slow=50 observed_ms=1200 longest_ms=1000 episodes=1 active_ms=1000
[cadence] phase=play_end frames=120 slow=50 observed_ms=2200 longest_ms=1000 episodes=1 active_ms=0
[cadence] phase=interval frames=60 slow=0 observed_ms=1000 longest_ms=0 episodes=0 active_ms=0
'''
s=analyze(text)['segments'];assert len(s)==2
assert s[0]['complete_segment'] and s[0]['slow']==50 and s[0]['longest_ms']==1000 and s[0]['reported_hitch_events']==1
assert not s[1]['complete_segment'] and s[1]['slow']==0
assert analyze('[cadence] frames=')['segments']==[]
print('PASS stutter analysis: cumulative counters, run durations, pause/reset, active segment and partial records')
