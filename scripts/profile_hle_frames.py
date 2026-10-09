"""Compare matched uncapped frontend workloads; software Vulkan is not PS4 timing."""
import json,re,statistics,sys
from pathlib import Path
directory=Path(sys.argv[1]);result={"environment":"Docker / Lavapipe software Vulkan; CPU reference retained", "ps4_fps_prediction":False,"modes":{}}
modes=("cpu","vulkan-reference","vulkan") if (directory/"vulkan-reference-host.log").exists() else ("cpu","vulkan")
for mode in modes:
 text=(directory/(mode+"-host.log")).read_text()
 rows=[(int(f),float(ms),int(v)) for f,ms,v in re.findall(r"PROFILE frame=(\d+) ms=([\d.]+) vulkan=(\d+)",text)]
 rows=[r for r in rows if 650<=r[0]<=3900]
 assert rows and all(r[2]==(mode!="cpu") for r in rows),mode
 times=sorted(r[1] for r in rows)
 result["modes"][mode]={"samples":len(times),"mean_ms":statistics.mean(times),"median_ms":statistics.median(times),"p95_ms":times[int(.95*(len(times)-1))],"p99_ms":times[int(.99*(len(times)-1))],"uncapped_fps":1000/statistics.mean(times),"frames_above_20ms_pct":100*sum(t>20 for t in times)/len(times)}
assert result["modes"]["cpu"]["samples"]==result["modes"]["vulkan"]["samples"]
cpu_states=re.findall(r"^STATE .*$",(directory/"cpu-host.log").read_text(),re.M)
gpu_states=re.findall(r"^STATE .*$",(directory/"vulkan-host.log").read_text(),re.M)
assert cpu_states and cpu_states==gpu_states,"CPU/Vulkan emulated states diverged"
for mode in modes:
 assert re.findall(r"^STATE .*$",(directory/(mode+"-host.log")).read_text(),re.M)==cpu_states,mode
 assert result["modes"][mode]["samples"]==len(rows),mode
result["matched_core_state_samples"]=len(cpu_states)
(directory/"performance.json").write_text(json.dumps(result,indent=2)+"\n")
print(json.dumps(result,indent=2))
