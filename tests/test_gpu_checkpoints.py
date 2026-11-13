"""Interleaved calls, retained crash boundary, and append-only session isolation."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from analyze_gpu_checkpoints import analyze

text = '''NPS64 Vulkan Game Test 00.02; previous failure
[gpu-api] id=55 begin vkQueueSubmit count=1
NPS64 Vulkan Game Test 00.03; new session
[run] frame=33 begin
[gpu-api] id=1 begin vkCreateComputePipelines count=1
[gpu-api] id=2 begin vkQueueSubmit count=1
[gpu-api] id=1 end vkCreateComputePipelines result=0
[gpu-api] id=3 begin vkCmdDispatchIndirect offset=16
[gpu-api] id=3 end vkCmdDispatchIndirect
'''
report = analyze(text)
assert report['entries'] == 3 and report['returns'] == 2
assert report['pending_calls'] == [{'id': 2, 'api': 'vkQueueSubmit', 'details': 'count=1'}]
assert report['last_core_checkpoint'] == ('33', 'begin')
assert not report['log_errors']
assert analyze(text + '[gpu-api] id=2 end vkQueueSubmit result=-4\n')['pending_calls'] == []
assert analyze('[gpu-api] id=1 end vkQueueSubmit result=0')['log_errors']
assert analyze('[gpu-api] id=1 begin vkQueueSubmit\n[gpu-api] id=1 end vkWaitForFences\n')['log_errors']
print('PASS Vulkan checkpoint parser: interleaving, pending call, failed return, session isolation, malformed pairing')
