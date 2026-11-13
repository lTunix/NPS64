#!/usr/bin/env python3
"""Record portable specialization recipes; replay after first cold readback."""
from pathlib import Path
import shutil,sys
root=Path(sys.argv[1]).resolve();project=Path(__file__).resolve().parents[1]
if root==project/'vendor/parallel-n64':raise SystemExit('Use a build copy, not vendor')
f=root/'mupen64plus-video-paraLLEl/parallel-rdp/vulkan'
for name in ['gpu_recipes.h','gpu_cache_store.h']:
 data=(project/'src'/name).read_bytes();target=f/name
 if not target.exists() or target.read_bytes()!=data:target.write_bytes(data)
p=f/'device.hpp';s=p.read_text()
if 'nps64_record_recipe' not in s:
 s='#include "gpu_recipes.h"\n#include <mutex>\n'+s
 anchor='\tProgram *request_program(Shader *compute);';assert s.count(anchor)==1
 s=s.replace(anchor,anchor+'''
    void nps64_record_recipe(const DeferredPipelineCompile &compile, uint64_t shader, uint32_t mask);
private:
    std::mutex nps64_recipe_mutex;
    std::vector<Nps64GpuRecipe> nps64_recipes;
    size_t nps64_loaded_recipes = 0;
    bool nps64_replaying = false;
    bool nps64_recipes_dirty = false;
    void nps64_init_recipes();
    void nps64_save_recipes();
    unsigned nps64_prepare_recipes();
public:
''');p.write_text(s)
p=f/'command_buffer.hpp';s=p.read_text()
if 'nps64_prewarm_recipe' not in s:
 s='#include "gpu_recipes.h"\n'+s
 anchor='\tstatic Pipeline build_compute_pipeline(Device *device, const DeferredPipelineCompile &compile, CompileMode mode);';assert s.count(anchor)==1
 s=s.replace(anchor,anchor+'''
    static bool nps64_prewarm_recipe(Device *device, Program *program, VkPipelineCache cache, const Nps64GpuRecipe &recipe);
''');p.write_text(s)
p=f/'command_buffer.cpp';s=p.read_text()
if 'bool CommandBuffer::nps64_prewarm_recipe' not in s:
 anchor='\tauto returned_pipeline = compile.program->add_pipeline(compile.hash, { compute_pipeline, 0 });';assert s.count(anchor)==1
 s=s.replace(anchor,'    device->nps64_record_recipe(compile, shader.get_hash(), mask);\n'+anchor)
 anchor='void CommandBuffer::extract_pipeline_state(DeferredPipelineCompile &compile) const';assert s.count(anchor)==1
 helper='''bool CommandBuffer::nps64_prewarm_recipe(Device *device, Program *program, VkPipelineCache cache,
                                         const Nps64GpuRecipe &recipe)
{
    static_assert(VULKAN_NUM_TOTAL_SPEC_CONSTANTS == NPS64_RECIPE_CONSTANTS, "Review recipe constants");
    DeferredPipelineCompile compile = {};
    compile.program=program;compile.cache=cache;
    const auto &layout=program->get_pipeline_layout()->get_resource_layout();
    if (recipe.mask & ~layout.combined_spec_constant_mask)return false;
    compile.potential_static_state.spec_constant_mask=recipe.mask & 0xff;
    compile.potential_static_state.internal_spec_constant_mask=(recipe.mask >> 8) & 15;
    memcpy(compile.potential_static_state.spec_constants,recipe.constants,sizeof(recipe.constants));
    update_hash_compute_pipeline(compile);
    if (compile.hash != recipe.pipeline)return false;
    if (program->get_pipeline(compile.hash).pipeline)return true;
    return build_compute_pipeline(device,compile,CompileMode::Sync).pipeline != VK_NULL_HANDLE;
}

'''
 s=s.replace(anchor,helper+anchor);p.write_text(s)
p=f/'device.cpp';s=p.read_text()
if 'void Device::nps64_init_recipes()' not in s:
 anchor='void Device::init_pipeline_cache()\n{';assert s.count(anchor)==1
 s=s.replace(anchor,anchor+'\n    nps64_init_recipes();')
 # Recipe saves happen independently of the disabled opaque driver cache.
 anchor='Device::~Device()\n{';assert s.count(anchor)==1
 s=s.replace(anchor,anchor+'\n    nps64_gpu_recipes_unbind(this);\n    nps64_save_recipes();')
 anchor='void Device::init_pipeline_cache()';pos=s.index(anchor)
 helper='''void Device::nps64_init_recipes()
{
    if (const char *dir=getenv("NPS64_PIPELINE_RECIPE_DIR"))
    {
        auto key=nps64_cache_identity(gpu_props);key.kind=3;
        Nps64GpuRecipe *data=nullptr;size_t count=0;
        bool loaded=nps64_gpu_recipes_load(dir,&key,&data,&count);
        if (loaded)nps64_recipes.assign(data,data+count);
        free(data);nps64_loaded_recipes=nps64_recipes.size();
        nps64_gpu_recipes_bind(this,[](void *p)->unsigned { return static_cast<Device*>(p)->nps64_prepare_recipes(); });
        LOGI("[pipeline-recipes] init loaded=%u count=%zu raw_binary_restore=0\\n",unsigned(loaded),count);
    }
}

void Device::nps64_record_recipe(const DeferredPipelineCompile &compile,uint64_t shader,uint32_t mask)
{
    if (!getenv("NPS64_PIPELINE_RECIPE_DIR") || compile.static_state.state.subgroup_control_size ||
        (mask >> NPS64_RECIPE_CONSTANTS))return;
    std::lock_guard<std::mutex> guard(nps64_recipe_mutex);
    if (nps64_replaying)return;
    for (const auto &r:nps64_recipes)if (r.shader==shader && r.pipeline==compile.hash)return;
    if (nps64_recipes.size()>=NPS64_RECIPE_LIMIT)return;
    Nps64GpuRecipe recipe={};recipe.shader=shader;recipe.pipeline=compile.hash;recipe.mask=mask;
    for (unsigned k=0;k<NPS64_RECIPE_CONSTANTS;k++)
        if (mask & (1u<<k))recipe.constants[k]=compile.potential_static_state.spec_constants[k];
    nps64_recipes.push_back(recipe);nps64_recipes_dirty=true;
}

unsigned Device::nps64_prepare_recipes()
{
    // Called by the frontend after retro_run returns and its first readback
    // fence completed. The core is suspended; this only creates pipelines.
    std::unique_lock<std::mutex> guard(nps64_recipe_mutex);
    if (!nps64_loaded_recipes)return 0;
    const char *enabled=getenv("NPS64_RECIPE_PREWARM");
    if (enabled && strcmp(enabled,"0")==0)return 0;
    const auto start=Util::get_current_time_nsecs();
    nps64_replaying=true;guard.unlock();unsigned prepared=0,skipped=0;
    LOGI("[pipeline-recipes] warm begin count=%zu budget_s=20 after_first_readback=1\\n",nps64_loaded_recipes);
    for (size_t i=0;i<nps64_loaded_recipes;i++)
    {
        if (Util::get_current_time_nsecs()-start>=20000000000ull)break;
        const auto &r=nps64_recipes[i];Util::Hasher hash;hash.u64(r.shader);
        Program *program=programs.find(hash.get());
        if (!program || !program->get_shader(ShaderStage::Compute) ||
            program->get_shader(ShaderStage::Compute)->get_hash()!=r.shader){skipped++;continue;}
        if (CommandBuffer::nps64_prewarm_recipe(this,program,pipeline_cache,r))prepared++;
        else {skipped++;LOGW("[pipeline-recipes] incompatible or rejected pipeline=%016llx\\n",static_cast<unsigned long long>(r.pipeline));}
    }
    guard.lock();nps64_replaying=false;
    LOGI("[pipeline-recipes] warm end prepared=%u skipped=%u available=%zu elapsed_ms=%.3f\\n",
         prepared,skipped,nps64_loaded_recipes,double(Util::get_current_time_nsecs()-start)/1000000.0);
    return prepared;
}

void Device::nps64_save_recipes()
{
    if (const char *dir=getenv("NPS64_PIPELINE_RECIPE_DIR"))
    {
        std::lock_guard<std::mutex> guard(nps64_recipe_mutex);
        if (!nps64_recipes_dirty || nps64_recipes.empty())return;
        auto key=nps64_cache_identity(gpu_props);key.kind=3;
        bool ok=nps64_gpu_recipes_save(dir,&key,nps64_recipes.data(),nps64_recipes.size());
        LOGI("[pipeline-recipes] save count=%zu ok=%u\\n",nps64_recipes.size(),unsigned(ok));
    }
}

'''
 s=s[:pos]+helper+s[pos:];p.write_text(s)
print('Portable pipeline recipes ready in '+str(root))
