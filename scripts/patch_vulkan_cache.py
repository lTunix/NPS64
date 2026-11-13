#!/usr/bin/env python3
"""Enable bounded local pipeline-cache persistence in a prepared build copy."""
from pathlib import Path
import shutil,sys
project=Path(__file__).resolve().parents[1]
root=Path(sys.argv[1]).resolve()
if root==project/'vendor/parallel-n64':raise SystemExit('Use a build copy, not vendor')
folder=root/'mupen64plus-video-paraLLEl/parallel-rdp/vulkan'
cache_header=(project/'src/gpu_cache_store.h').read_bytes()
cache_target=folder/'nps64_gpu_cache_store.h'
if not cache_target.exists() or cache_target.read_bytes()!=cache_header:cache_target.write_bytes(cache_header)
path=folder/'device.cpp';text=path.read_text();marker='// NPS64 bounded pipeline cache'
if marker not in text:
    text='#include "nps64_gpu_cache_store.h"\n#include <cstdlib>\n'+text
    anchor='void Device::init_pipeline_cache()\n{'
    assert text.count(anchor)==1
    key='''// NPS64 bounded pipeline cache. Driver identity and an outer SHA256
// are checked before passing persisted bytes to Vulkan.
static Nps64GpuCacheIdentity nps64_cache_identity(const VkPhysicalDeviceProperties &p)
{
    Nps64GpuCacheIdentity key = {};
    key.vendor=p.vendorID;key.device=p.deviceID;key.driver=p.driverVersion;key.kind=1;
    memcpy(key.uuid,p.pipelineCacheUUID,16);return key;
}
'''
    text=text.replace(anchor,key+anchor+'''
    if (const char *directory=getenv("NPS64_PIPELINE_CACHE_DIR"))
    {
        void *data=nullptr;size_t size=0;
        const auto key=nps64_cache_identity(gpu_props);
        bool loaded=nps64_gpu_cache_load(directory,&key,&data,&size);
        bool ok=loaded ? init_pipeline_cache(static_cast<const uint8_t*>(data),size) : false;
        free(data);
        if (!ok)ok=init_pipeline_cache(nullptr,0);
        LOGI("[pipeline-cache] init loaded=%u bytes=%zu ready=%u\\n",unsigned(loaded),size,unsigned(ok));
        return;
    }
''')
    anchor='void Device::flush_pipeline_cache()\n{';assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'''
    if (const char *directory=getenv("NPS64_PIPELINE_CACHE_DIR"))
    {
        size_t size=get_pipeline_cache_size();
        if (!size || size>NPS64_GPU_CACHE_LIMIT)return;
        auto *data=static_cast<uint8_t*>(malloc(size));
        if (!data){LOGE("[pipeline-cache] allocation failed; cache unchanged\\n");return;}
        const auto key=nps64_cache_identity(gpu_props);
        bool saved=get_pipeline_cache_data(data,size) && nps64_gpu_cache_save(directory,&key,data,size);
        free(data);
        LOGI("[pipeline-cache] save bytes=%zu ok=%u\\n",size,unsigned(saved));
        return;
    }
''')
    path.write_text(text)
print('Pipeline cache persistence ready in '+str(root))

# Read back only on game exit, outside the frame loop. This distinguishes
# persisted-but-unreadable data on the console from a driver cache miss.
text=path.read_text()
if '[pipeline-cache] verify' not in text:
    old='        LOGI("[pipeline-cache] save bytes=%zu ok=%u\\n",size,unsigned(saved));\n        return;'
    new='        LOGI("[pipeline-cache] save bytes=%zu ok=%u\\n",size,unsigned(saved));\n        if (saved)\n        {\n            void *verify=nullptr;size_t verify_size=0;\n            bool readable=nps64_gpu_cache_load(directory,&key,&verify,&verify_size);\n            LOGI("[pipeline-cache] verify readable=%u bytes=%zu expected=%zu\\n",unsigned(readable),verify_size,size);\n            free(verify);\n        }\n        return;'
    assert text.count(old)==1, 'Review pipeline-cache verification anchor'
    path.write_text(text.replace(old,new))

# Test-title policy: a fresh run must not replace the cache reserved for repro.
text=path.read_text()
if 'NPS64_PIPELINE_CACHE_LOAD' not in text:
    old='        bool loaded=nps64_gpu_cache_load(directory,&key,&data,&size);'
    new='        const char *policy=getenv("NPS64_PIPELINE_CACHE_LOAD");\n        bool permitted=!policy || strcmp(policy,"0")!=0;\n        bool loaded=permitted && nps64_gpu_cache_load(directory,&key,&data,&size);'
    assert text.count(old)==1
    text=text.replace(old,new)
    anchor='        size_t size=get_pipeline_cache_size();'
    assert text.count(anchor)==1
    text=text.replace(anchor,'        const char *policy=getenv("NPS64_PIPELINE_CACHE_LOAD");\n        if (policy && strcmp(policy,"0")==0)\n        {\n            LOGI("[pipeline-cache] save skipped: fresh diagnostic run; existing disk cache preserved\\n");\n            return;\n        }\n' + anchor)
    path.write_text(text)
