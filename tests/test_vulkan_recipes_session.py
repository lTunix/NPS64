from pathlib import Path
import argparse
import os,json,re,shutil,subprocess,hashlib
parser=argparse.ArgumentParser(description='Compare learned portable variants with CPU prewarm disabled/enabled, using one prepared native binary and an isolated ROM fixture.')
parser.add_argument('--data',type=Path,required=True)
parser.add_argument('--binary',type=Path,default=Path('build/bench-vulkan-game-00.12-asan'))
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
root=args.data;out=args.output;out.mkdir(exist_ok=False)
snapshot=out/'saves';shutil.copytree(root/'vulkan-test-saves',snapshot)
recipes=out/'initial-recipes';shutil.copytree(root/'pipeline-recipes',recipes)
raw=root/'pipeline-cache';raw_fingerprint={p.name:[p.stat().st_mtime_ns,hashlib.sha256(p.read_bytes()).hexdigest()] for p in raw.glob('*') if p.is_file()}
env=dict(os.environ,BENCH_FRAMES='1000000',NPS64_GPU_NO_DEEPBIND='1',ASAN_OPTIONS='detect_leaks=0',VK_DRIVER_FILES='/usr/share/vulkan/icd.d/lvp_icd.json',VK_LAYER_PATH='/work/build/tools/usr/share/vulkan/explicit_layer.d',VK_INSTANCE_LAYERS='VK_LAYER_KHRONOS_validation',NPS64_CPU_FRAME_HASHES='1',LP_NUM_THREADS='2')
rows=[];images=[]
for mode in ['off','on']:
 for p in snapshot.glob('*'):shutil.copyfile(p,root/'vulkan-test-saves'/p.name)
 shutil.copytree(recipes,root/'pipeline-recipes',dirs_exist_ok=True)
 env['NPS64_RECIPE_PREWARM']='0' if mode=='off' else '1'
 with (out/(mode+'-driver.log')).open('w') as stream:subprocess.run([str(args.binary.resolve())],env=env,stdout=stream,stderr=subprocess.STDOUT,check=True,timeout=180)
 driver=(out/(mode+'-driver.log')).read_text();core=(root/'vulkan-game-test.log').read_text().split('NPS64 Vulkan Game Test 00.12;')[-1];(out/(mode+'-core.log')).write_text(core)
 assert 'ERROR: AddressSanitizer' not in driver and 'VUID-' not in driver
 assert '[pipeline-cache] init loaded=0 bytes=0 ready=1' in driver and '[pipeline-cache] save skipped:' in driver
 assert '[pipeline-recipes] init loaded=1' in driver
 assert '[gpu-test] LIMIT reached' in core and 'limit_reached=1' in core
 if mode=='on':
  m=re.search(r'warm end prepared=(\d+) skipped=(\d+) available=(\d+) elapsed_ms=([\d.]+)',driver);assert m and int(m[1])>1 and int(m[2])==0
  before,after=driver.split('[pipeline-recipes] warm begin',1);warming,game=after.split('[pipeline-recipes] warm end',1)
  row={'mode':mode,'prepared':int(m[1]),'available':int(m[3]),'warm_ms':float(m[4]),'runtime_compiles':before.count('[pipeline-compile] begin')+game.count('[pipeline-compile] begin'),'preload_compiles':warming.count('[pipeline-compile] begin')}
 else:row={'mode':mode,'runtime_compiles':driver.count('[pipeline-compile] begin')}
 images.append(dict(re.findall(r'\[cpu-image\] frame=(\d+) value=(\w+)',core)));rows.append(row);print(json.dumps(row),flush=True)
common=images[0].keys()&images[1].keys();assert len(common)>180 and all(images[0][k]==images[1][k] for k in common)
assert rows[1]['runtime_compiles']<rows[0]['runtime_compiles']
assert raw_fingerprint=={p.name:[p.stat().st_mtime_ns,hashlib.sha256(p.read_bytes()).hexdigest()] for p in raw.glob('*') if p.is_file()}
report={'runs':rows,'equal_image_hashes':len(common),'raw_cache_untouched':True,'note':'Host functional/compilation placement test; PS4 performance pending.'};(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
