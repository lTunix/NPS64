/* N64 rasterization runs in paraLLEl-RDP compute shaders. This first frontend
 * transfers the native GPU scanout to the existing cached canvas/VideoOut.
 * SPDX-License-Identifier: GPL-2.0-or-later */
#include "gpu.h"
#ifdef NPS64_GPU_PRESENT_TEST
#include "gpu_presenter.h"
#endif
#include "log.h"
#include "platform.h"
#include <libretro_vulkan.h>
#include "gpu_readback_memory.h"
#include <string.h>
#include <stdlib.h>
#ifdef NPS64_GPU_GAME_TEST
#include "gpu_api_trace.h"
#endif
#if defined(__ORBIS__) && defined(NPS64_GPU_GAME_TEST)
#include "gpu_pipeline_guard.h"
#endif
#ifdef __ORBIS__
#include "gpu_capture.h"
extern PFN_vkVoidFunction vk_icdGetInstanceProcAddr(VkInstance,const char *);
#else
#include <dlfcn.h>
#endif
extern int nps64_parallel_ready(void);
static Nps64GpuPerf gpu_perf;
void nps64_gpu_perf_take(Nps64GpuPerf *out){*out=gpu_perf;memset(&gpu_perf,0,sizeof(gpu_perf));}
void nps64_gpu_perf_snapshot(double seconds){gpu_perf.snapshot_seconds+=seconds;}
static struct {
    VkInstance instance;
    struct retro_vulkan_context context;
    struct retro_hw_render_callback hw;
    struct retro_hw_render_context_negotiation_interface_vulkan negotiation;
    struct retro_hw_render_interface_vulkan interface;
    PFN_vkGetInstanceProcAddr gipa;
    PFN_vkGetDeviceProcAddr gdpa;
    VkCommandPool pool;
    VkCommandBuffer command;
    VkFence fence;
    VkBuffer buffer;
    VkDeviceMemory memory;
    void *pixels;
    VkDeviceSize capacity;
    bool coherent,reset_called,failed,first_frame;
#ifdef NPS64_GPU_PRESENT_TEST
    Nps64Presenter *presenter;bool present_attempted,present_external;
    unsigned present_index;VkImageView image_view;
#endif
    VkImage image;
    VkImageLayout layout;
    VkFormat format;
    uint32_t source_family;
    VkSemaphore waits[8],signal;
    unsigned wait_count;
    VkCommandBuffer extra_commands[8];
    unsigned extra_count;
    unsigned queue_lock,scanout_width,scanout_height;
#ifndef __ORBIS__
    void *loader;
#endif
} g;
#define DEVICE_FUNCTIONS(X) \
 X(DestroyDevice) X(DeviceWaitIdle) X(GetBufferMemoryRequirements) X(CreateBuffer) X(DestroyBuffer) \
 X(AllocateMemory) X(FreeMemory) X(BindBufferMemory) X(MapMemory) X(UnmapMemory) \
 X(InvalidateMappedMemoryRanges) X(CreateCommandPool) X(DestroyCommandPool) \
 X(AllocateCommandBuffers) X(ResetCommandPool) X(BeginCommandBuffer) X(EndCommandBuffer) \
 X(CmdPipelineBarrier) X(CmdCopyImageToBuffer) X(CreateFence) X(DestroyFence) X(ResetFences) \
 X(WaitForFences) X(QueueSubmit)
#define DECLARE(name) static PFN_vk##name fn_##name;
DEVICE_FUNCTIONS(DECLARE)
#undef DECLARE
static PFN_vkDestroyInstance fn_DestroyInstance;
static PFN_vkGetPhysicalDeviceMemoryProperties fn_GetPhysicalDeviceMemoryProperties;
static bool check(VkResult result,const char *operation) {
    if(result==VK_SUCCESS)return true;
    nps64_log("[gpu] %s failed VkResult=%d\n",operation,result);g.failed=true;return false;
}
#define CHECK(call) do {if(!check((call),#call))return false;} while(0)
static void lock_queue(void *unused) {
    (void)unused;
    while(__atomic_exchange_n(&g.queue_lock,1,__ATOMIC_ACQUIRE)) __asm__ volatile("pause");
}
static void unlock_queue(void *unused) {(void)unused;__atomic_store_n(&g.queue_lock,0,__ATOMIC_RELEASE);}
static uint32_t sync_index(void *unused) {(void)unused;return 0;}
static uint32_t sync_mask(void *unused) {(void)unused;return 1;}
static void wait_sync(void *unused) {(void)unused; /* readback completed its fence before returning */}
static void set_signal(void *unused,VkSemaphore semaphore) {(void)unused;g.signal=semaphore;}
static void set_commands(void *unused,uint32_t count,const VkCommandBuffer *commands) {
    (void)unused;
    if(count>8){g.failed=true;return;}
    if(count)memcpy(g.extra_commands,commands,count*sizeof(*commands));g.extra_count=count;
}
static void set_image(void *unused,const struct retro_vulkan_image *image,uint32_t count,
                      const VkSemaphore *semaphores,uint32_t family) {
    (void)unused;
    if(!image || count>8){g.failed=true;return;}
#ifdef NPS64_GPU_PRESENT_TEST
    nps64_presenter_invalidate(g.presenter);
    g.image_view=image->image_view;
#endif
    g.image=image->create_info.image;g.layout=image->image_layout;g.format=image->create_info.format;
    g.source_family=family;g.wait_count=count;
    if(count)memcpy(g.waits,semaphores,count*sizeof(*semaphores));
}
bool nps64_gpu_begin(void) {
#if defined(__ORBIS__) && !defined(NPS64_GPU_EXPERIMENTAL) && !defined(NPS64_GPU_GAME_TEST)
    /* Defense in depth: never initialize GNM on console while quarantined. */
    nps64_log("[gpu] PS4 Vulkan disabled: unresolved system-wide GPU hang\n");
    return false;
#endif
#ifdef NPS64_GPU_GAME_TEST
    const char *trace=getenv("NPS64_GPU_API_TRACE");
    bool detailed_trace=trace && !strcmp(trace,"1");
    if(!detailed_trace && (unsetenv("NPS64_GPU_API_TRACE") || unsetenv("RADV_PS4_TRACE")))return false;
    if(detailed_trace && setenv("RADV_PS4_TRACE","1",1))return false;
    const char *uber=getenv("NPS64_RDP_GAME_UBERSHADER");
    const char *sync=getenv("NPS64_RDP_SYNC_SHADER");
#ifdef NPS64_RDP_STAGE_TIMINGS
    if(setenv("PARALLEL_RDP_BENCH","2",1))return false;
    nps64_log("[gpu-stage] diagnostic=1 timestamp_queries=1; performance comparison requires profiling off\n");
#endif
    /* ACO scheduling off keeps the general split shader within the guard. */
    if(setenv("ACO_DEBUG","nosched",1))return false;
    nps64_log("[gpu] compiler profile ACO_DEBUG=nosched; resource limits unchanged\n");
    if(!sync)sync="0";
    if(strcmp(sync,"0") && strcmp(sync,"1"))return false;
    if(!uber)uber="0"; /* Evaluate split stages after the full ubershader hang. */
    if(strcmp(uber,"0") && strcmp(uber,"1"))return false;
#ifdef __ORBIS__
    /* The first game test lost its hang report because the parent directory
     * did not exist. Refuse GPU startup unless capture storage is writable. */
    if(!nps64_gpu_capture_prepare("/data/NPS64")) {
        nps64_log("[gpu] capture storage unavailable; refusing GPU test\n");return false;
    }
#endif
    /* Separate test title, after the physical compute probes passed. Use
     * the complete RDP pipeline, not the old color-only diagnostic. */
    if(setenv("NPS64_RDP_DIRECT_BINNING","0",1) ||
       setenv("NPS64_RDP_COLOR_ONLY","0",1) ||
       setenv("PARALLEL_RDP_UBERSHADER",uber,1) ||
       setenv("PARALLEL_RDP_SUBGROUP","0",1) ||
       setenv("PARALLEL_RDP_SMALL_TYPES","0",1) ||
       setenv("PARALLEL_RDP_FORCE_SYNC_SHADER",sync,1) ||
       setenv("NPS64_RDP_SERIALIZE_SETUP","1",1) ||
       setenv("PARALLEL_RDP_ALLOW_EXTERNAL_HOST","0",1) ||
       setenv("NPS64_GPU_COMPILE_WORKER","1",1))return false;
    nps64_log("[gpu] detailed_trace=%u; external host import disabled (driver returns ENOSYS)\n",detailed_trace);
    nps64_log("[gpu] GAME TEST full RDP; ubershader=%s sync_shader=%s subgroup=0 small_types=0; newer GNM backend\n",uber,sync);
#elif defined(__ORBIS__) || defined(NPS64_TEST_PS4_GPU_PROFILE)
    /* Host can select the upstream shader for exact image comparisons. */
#ifdef __ORBIS__
    if(setenv("NPS64_RDP_DIRECT_BINNING","1",1))return false;
    if(setenv("NPS64_RDP_COLOR_ONLY","1",1))return false;
#else
    if(!getenv("NPS64_RDP_DIRECT_BINNING") && setenv("NPS64_RDP_DIRECT_BINNING","1",1))return false;
    if(!getenv("NPS64_RDP_COLOR_ONLY") && setenv("NPS64_RDP_COLOR_ONLY","1",1))return false;
#endif
    /* The first real RDP compute workload hangs with both specialized and
       direct dispatch. Retain the direct path while diagnosing GNM visibility. */
    if(setenv("PARALLEL_RDP_UBERSHADER","1",1) ||
       setenv("PARALLEL_RDP_SUBGROUP","0",1) ||
       setenv("PARALLEL_RDP_SMALL_TYPES","0",1) ||
       setenv("PARALLEL_RDP_FORCE_SYNC_SHADER","1",1) ||
       setenv("NPS64_RDP_SERIALIZE_SETUP","1",1) ||
       setenv("RADV_PS4_SERIAL_SUBMIT","1",1) ||
       setenv("RADV_PS4_TRACE","1",1)) {
        nps64_log("[gpu] failed to configure PS4 compute profile\n");return false;
    }
    nps64_log("[gpu] PS4 RADV progress markers enabled\n");
    nps64_log("[gpu] conservative compute: serialized setup and single-flight submissions\n");
    nps64_log("[gpu] PS4 compute profile: ubershader=1 direct dispatch; subgroup=0 small_types=0\n");
#endif
#ifdef __ORBIS__
    extern void nps64_gpu_static_init(void);
    nps64_log("[gpu] deferred C++ initialization begin\n");
    nps64_gpu_static_init();
    nps64_log("[gpu] deferred C++ initialization complete\n");
#endif
    memset(&g,0,sizeof(g));
    memset(&gpu_perf,0,sizeof(gpu_perf));
#define RESET(name) fn_##name=NULL;
    DEVICE_FUNCTIONS(RESET)
#undef RESET
    fn_DestroyInstance=NULL;fn_GetPhysicalDeviceMemoryProperties=NULL;
#ifdef __ORBIS__
    g.gipa=vk_icdGetInstanceProcAddr;
#ifdef NPS64_GPU_GAME_TEST
    g.gipa=nps64_pipeline_guard_wrap(g.gipa);
    nps64_log("[gpu] compute resource guard: scratch=0 spills=0 SGPR<=96 VGPR<=128 LDS<=512\n");
#endif
#else
    /* ASan must interpose allocation in the native diagnostic. */
    int loader_flags=RTLD_NOW|RTLD_LOCAL;
    if(!getenv("NPS64_GPU_NO_DEEPBIND"))loader_flags|=RTLD_DEEPBIND;
    g.loader=dlopen("libvulkan.so.1",loader_flags);
    if(g.loader)g.gipa=(PFN_vkGetInstanceProcAddr)dlsym(g.loader,"vkGetInstanceProcAddr");
#endif
#ifdef NPS64_GPU_GAME_TEST
    if(detailed_trace)g.gipa=nps64_gpu_api_trace_wrap(g.gipa);
#ifdef NPS64_HOST_COMPILE_WORKER_TEST
    else g.gipa=nps64_gpu_api_trace_wrap(g.gipa); /* Native worker test adapter. */
#endif
#endif
    if(!g.gipa){nps64_log("[gpu] Vulkan loader unavailable\n");return false;}
    PFN_vkCreateInstance create=(PFN_vkCreateInstance)g.gipa(VK_NULL_HANDLE,"vkCreateInstance");
    if(!create){nps64_gpu_close();return false;}
    VkApplicationInfo app={.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,.pApplicationName="NPS64",
                           .pEngineName="paraLLEl-RDP",.apiVersion=VK_API_VERSION_1_1};
    VkInstanceCreateInfo info={.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,.pApplicationInfo=&app};
    if(!check(create(&info,NULL,&g.instance),"vkCreateInstance")){nps64_gpu_close();return false;}
    fn_DestroyInstance=(PFN_vkDestroyInstance)g.gipa(g.instance,"vkDestroyInstance");
    fn_GetPhysicalDeviceMemoryProperties=(PFN_vkGetPhysicalDeviceMemoryProperties)g.gipa(g.instance,"vkGetPhysicalDeviceMemoryProperties");
    nps64_log("[gpu] Vulkan instance ready\n");return true;
}
bool nps64_gpu_environment(unsigned command,void *data) {
    if(!g.instance)return false;
    switch(command) {
    case RETRO_ENVIRONMENT_SET_HW_RENDER: {
        struct retro_hw_render_callback *hw=data;
        if(hw->context_type!=RETRO_HW_CONTEXT_VULKAN)return false;
        g.hw=*hw;return true;
    }
    case RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE: {
        const struct retro_hw_render_context_negotiation_interface_vulkan *n=data;
        if(n->interface_type!=RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_VULKAN ||
           n->interface_version!=1 || !n->create_device)return false;
        g.negotiation=*n;return true;
    }
    case RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE:
        if(!g.context.device)return false;
        *(const struct retro_hw_render_interface_vulkan**)data=&g.interface;return true;
    case RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER:
        *(unsigned*)data=RETRO_HW_CONTEXT_VULKAN;return true;
    default:return false;
    }
}
bool nps64_gpu_start(void) {
    if(!g.instance || !g.negotiation.create_device || !g.hw.context_reset)return false;
    VkPhysicalDeviceFeatures features={0};
    if(!g.negotiation.create_device(&g.context,g.instance,VK_NULL_HANDLE,VK_NULL_HANDLE,g.gipa,
                                  NULL,0,NULL,0,&features)) {
        nps64_log("[gpu] core device negotiation failed\n");return false;
    }
    g.gdpa=(PFN_vkGetDeviceProcAddr)g.gipa(g.instance,"vkGetDeviceProcAddr");
#define LOAD(name) fn_##name=(PFN_vk##name)g.gdpa(g.context.device,"vk" #name);if(!fn_##name)return false;
    DEVICE_FUNCTIONS(LOAD)
#undef LOAD
    g.interface=(struct retro_hw_render_interface_vulkan){
        .interface_type=RETRO_HW_RENDER_INTERFACE_VULKAN,.interface_version=5,
        .instance=g.instance,.gpu=g.context.gpu,.device=g.context.device,
        .get_device_proc_addr=g.gdpa,.get_instance_proc_addr=g.gipa,.queue=g.context.queue,
        .queue_index=g.context.queue_family_index,.set_image=set_image,.get_sync_index=sync_index,
        .get_sync_index_mask=sync_mask,.set_command_buffers=set_commands,.wait_sync_index=wait_sync,
        .lock_queue=lock_queue,.unlock_queue=unlock_queue,.set_signal_semaphore=set_signal};
    VkCommandPoolCreateInfo pool={.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                  .queueFamilyIndex=g.context.queue_family_index};
    CHECK(fn_CreateCommandPool(g.context.device,&pool,NULL,&g.pool));
    VkCommandBufferAllocateInfo alloc={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool=g.pool,.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,.commandBufferCount=1};
    CHECK(fn_AllocateCommandBuffers(g.context.device,&alloc,&g.command));
    VkFenceCreateInfo fence={.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    CHECK(fn_CreateFence(g.context.device,&fence,NULL,&g.fence));
    PFN_vkGetPhysicalDeviceProperties props=(PFN_vkGetPhysicalDeviceProperties)g.gipa(g.instance,"vkGetPhysicalDeviceProperties");
    VkPhysicalDeviceProperties properties;props(g.context.gpu,&properties);
    nps64_log("[gpu] device=%s type=%u queue=%u\n",properties.deviceName,properties.deviceType,g.context.queue_family_index);
    g.reset_called=true;g.hw.context_reset();
    if(nps64_gpu_failed()){nps64_log("[gpu] initial shader profile rejected; refusing game submission\n");return false;}
    if(!nps64_parallel_ready()){nps64_log("[gpu] paraLLEl-RDP feature check failed\n");return false;}
    nps64_log("[gpu] renderer=paraLLEl-RDP active\n");return true;
}
static void free_buffer(void) {
    if(g.pixels)fn_UnmapMemory(g.context.device,g.memory);
    if(g.buffer)fn_DestroyBuffer(g.context.device,g.buffer,NULL);
    if(g.memory)fn_FreeMemory(g.context.device,g.memory,NULL);
    g.pixels=NULL;g.buffer=VK_NULL_HANDLE;g.memory=VK_NULL_HANDLE;g.capacity=0;
}
static bool ensure_buffer(VkDeviceSize size) {
    if(size<=g.capacity)return true;
    free_buffer();
    VkBufferCreateInfo info={.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,.size=size,
        .usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT,.sharingMode=VK_SHARING_MODE_EXCLUSIVE};
    CHECK(fn_CreateBuffer(g.context.device,&info,NULL,&g.buffer));
    VkMemoryRequirements requirements;fn_GetBufferMemoryRequirements(g.context.device,g.buffer,&requirements);
    VkPhysicalDeviceMemoryProperties properties;fn_GetPhysicalDeviceMemoryProperties(g.context.gpu,&properties);
    unsigned type=nps64_readback_memory_type(&properties,requirements.memoryTypeBits);
    if(type==UINT32_MAX){nps64_log("[gpu] no host-visible transfer memory\n");g.failed=true;return false;}
    g.coherent=(properties.memoryTypes[type].propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)!=0;
    nps64_log("[gpu-readback] memory_type=%u flags=0x%x cached=%u coherent=%u buffer_bytes=%llu\n",type,
        properties.memoryTypes[type].propertyFlags,
        !!(properties.memoryTypes[type].propertyFlags&VK_MEMORY_PROPERTY_HOST_CACHED_BIT),g.coherent,
        (unsigned long long)requirements.size);
    VkMemoryAllocateInfo alloc={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize=requirements.size,.memoryTypeIndex=type};
    if(!g.first_frame)nps64_log("[readback-start] allocate begin\n");
    CHECK(fn_AllocateMemory(g.context.device,&alloc,NULL,&g.memory));
    if(!g.first_frame)nps64_log("[readback-start] allocate end; bind begin\n");
    CHECK(fn_BindBufferMemory(g.context.device,g.buffer,g.memory,0));
    if(!g.first_frame)nps64_log("[readback-start] bind end; map begin\n");
    CHECK(fn_MapMemory(g.context.device,g.memory,0,VK_WHOLE_SIZE,0,&g.pixels));
    if(!g.first_frame)nps64_log("[readback-start] map end\n");
    g.capacity=size;return true;
}
bool nps64_gpu_readback(const void **pixels,unsigned width,unsigned height) {
    if(nps64_gpu_failed() || !g.image || !width || !height || width>4096 || height>4096)return false;
    double started=platform_time();
    if(g.format!=VK_FORMAT_R8G8B8A8_UNORM && g.format!=VK_FORMAT_R8G8B8A8_SRGB) {
        nps64_log("[gpu] unsupported scanout format=%u\n",g.format);g.failed=true;return false;
    }
    if(!ensure_buffer((VkDeviceSize)width*height*4))return false;
    if(!g.first_frame)nps64_log("[readback-start] buffer ready; reset pool begin\n");
    CHECK(fn_ResetCommandPool(g.context.device,g.pool,0));
    VkCommandBufferBeginInfo begin={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                    .flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    if(!g.first_frame)nps64_log("[readback-start] begin command\n");
    CHECK(fn_BeginCommandBuffer(g.command,&begin));
    VkImageLayout copy_layout=g.layout==VK_IMAGE_LAYOUT_GENERAL?g.layout:VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    uint32_t src=g.source_family,dst=g.context.queue_family_index;
    if(src==VK_QUEUE_FAMILY_IGNORED || src==dst)src=dst=VK_QUEUE_FAMILY_IGNORED;
    VkImageMemoryBarrier image={.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask=VK_ACCESS_MEMORY_WRITE_BIT,.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT,
        .oldLayout=g.layout,.newLayout=copy_layout,.srcQueueFamilyIndex=src,.dstQueueFamilyIndex=dst,
        .image=g.image,.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}};
    fn_CmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
                          0,0,NULL,0,NULL,1,&image);
    VkBufferImageCopy copy={.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},.imageExtent={width,height,1}};
    fn_CmdCopyImageToBuffer(g.command,g.image,copy_layout,g.buffer,1,&copy);
    image.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;image.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
    image.oldLayout=copy_layout;image.newLayout=g.layout;image.srcQueueFamilyIndex=dst;image.dstQueueFamilyIndex=src;
    VkBufferMemoryBarrier buffer={.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,.dstAccessMask=VK_ACCESS_HOST_READ_BIT,
        .srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
        .buffer=g.buffer,.size=VK_WHOLE_SIZE};
    fn_CmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT|VK_PIPELINE_STAGE_HOST_BIT,0,0,NULL,1,&buffer,1,&image);
    if(!g.first_frame)nps64_log("[readback-start] end command; reset fence\n");
    CHECK(fn_EndCommandBuffer(g.command));CHECK(fn_ResetFences(g.context.device,1,&g.fence));
    VkPipelineStageFlags stages[8];for(unsigned i=0;i<g.wait_count;i++)stages[i]=VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkCommandBuffer commands[9];memcpy(commands,g.extra_commands,g.extra_count*sizeof(*commands));commands[g.extra_count]=g.command;
    VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.waitSemaphoreCount=g.wait_count,
        .pWaitSemaphores=g.waits,.pWaitDstStageMask=stages,.commandBufferCount=g.extra_count+1,.pCommandBuffers=commands,
        .signalSemaphoreCount=g.signal?1:0,.pSignalSemaphores=&g.signal};
    double recorded=platform_time();
    if(!g.first_frame)nps64_log("[readback-start] submit begin\n");
    lock_queue(NULL);VkResult result=fn_QueueSubmit(g.context.queue,1,&submit,g.fence);unlock_queue(NULL);
    if(!check(result,"vkQueueSubmit transfer"))return false;
    double submitted=platform_time();
    if(!g.first_frame)nps64_log("[readback-start] submit end; wait begin\n");
    CHECK(fn_WaitForFences(g.context.device,1,&g.fence,VK_TRUE,5000000000ULL));
    if(!g.first_frame)nps64_log("[readback-start] wait end\n");
    double completed=platform_time();
    if(!g.coherent) {
        VkMappedMemoryRange range={.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,.memory=g.memory,.size=VK_WHOLE_SIZE};
        CHECK(fn_InvalidateMappedMemoryRanges(g.context.device,1,&range));
    }
    double invalidated=platform_time();
    gpu_perf.readbacks++;gpu_perf.record_seconds+=recorded-started;
    gpu_perf.submit_seconds+=submitted-recorded;gpu_perf.wait_seconds+=completed-submitted;
    gpu_perf.invalidate_seconds+=invalidated-completed;gpu_perf.total_seconds+=invalidated-started;
    g.wait_count=g.extra_count=0;g.signal=VK_NULL_HANDLE;*pixels=g.pixels;
    if(!g.first_frame){g.first_frame=true;nps64_log("[gpu] first scanout %ux%u fence completed\n",width,height);}
    if(g.scanout_width!=width || g.scanout_height!=height) {
        g.scanout_width=width;g.scanout_height=height;
        nps64_log("[gpu] scanout geometry %ux%u RGBA format=%u\n",width,height,g.format);
    }
    return true;
}
int nps64_gpu_present(unsigned width,unsigned height,bool wide,bool filtered) {
#ifndef NPS64_GPU_PRESENT_TEST
    (void)width;(void)height;(void)wide;(void)filtered;return 0;
#else
    if(nps64_gpu_failed())return -1;
    if(!g.image||!g.image_view||!width||!height||width>4096||height>4096||g.format!=VK_FORMAT_R8G8B8A8_UNORM)return 0;
    if(!g.present_attempted){
        g.present_attempted=true;
        g.presenter=nps64_presenter_create(g.context.gpu,g.context.device,g.gipa,g.instance,g.gdpa);
        if(g.presenter&&platform_gpu_scanout_register){
            void *pixels[3];for(unsigned i=0;i<3;i++)pixels[i]=nps64_presenter_pixels(g.presenter,i);
            g.present_external=platform_gpu_scanout_register(pixels);
        }
#ifdef __ORBIS__
        /* Do not ship a potentially slower GPU+CPU-copy hybrid on console.
         * A rejected external registration keeps the validated legacy path. */
        if(g.presenter&&!g.present_external){nps64_presenter_destroy(g.presenter);g.presenter=NULL;}
#endif
        nps64_log("[gpu-present] enabled=%u direct_videoout=%u FullSync=unchanged\n",g.presenter!=NULL,g.present_external);
    }
    if(!g.presenter)return 0;
    double started=platform_time();
    void *pixels=nps64_presenter_pixels(g.presenter,g.present_index);
    if(g.present_external&&(!platform_gpu_scanout_acquire||!platform_gpu_scanout_acquire(pixels))){g.failed=true;return -1;}
    double acquired=platform_time();
    if(!check(fn_ResetCommandPool(g.context.device,g.pool,0),"present reset pool"))return -1;
    VkCommandBufferBeginInfo begin={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    if(!check(fn_BeginCommandBuffer(g.command,&begin),"present begin"))return -1;
    uint32_t src=g.source_family,dst=g.context.queue_family_index;
    if(src==VK_QUEUE_FAMILY_IGNORED||src==dst)src=dst=VK_QUEUE_FAMILY_IGNORED;
    VkImageMemoryBarrier image={.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_MEMORY_WRITE_BIT,.dstAccessMask=VK_ACCESS_SHADER_READ_BIT,.oldLayout=g.layout,.newLayout=g.layout,.srcQueueFamilyIndex=src,.dstQueueFamilyIndex=dst,.image=g.image,.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}};
    fn_CmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,0,NULL,0,NULL,1,&image);
    if(!nps64_presenter_record(g.presenter,g.command,g.image_view,g.layout,g.present_index,width,height,wide,filtered)){g.failed=true;return -1;}
    image.srcAccessMask=VK_ACCESS_SHADER_READ_BIT;image.dstAccessMask=VK_ACCESS_MEMORY_READ_BIT;image.srcQueueFamilyIndex=dst;image.dstQueueFamilyIndex=src;
    fn_CmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,0,NULL,0,NULL,1,&image);
    if(!check(fn_EndCommandBuffer(g.command),"present end")||!check(fn_ResetFences(g.context.device,1,&g.fence),"present reset fence"))return -1;
    VkPipelineStageFlags stages[8];for(unsigned i=0;i<g.wait_count;i++)stages[i]=VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    VkCommandBuffer commands[9];memcpy(commands,g.extra_commands,g.extra_count*sizeof(*commands));commands[g.extra_count]=g.command;
    VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.waitSemaphoreCount=g.wait_count,.pWaitSemaphores=g.waits,.pWaitDstStageMask=stages,.commandBufferCount=g.extra_count+1,.pCommandBuffers=commands,.signalSemaphoreCount=g.signal?1:0,.pSignalSemaphores=&g.signal};
    double recorded=platform_time();
    lock_queue(NULL);VkResult result=fn_QueueSubmit(g.context.queue,1,&submit,g.fence);unlock_queue(NULL);
    double submitted=platform_time();
    if(!check(result,"present submit")||!check(fn_WaitForFences(g.context.device,1,&g.fence,VK_TRUE,5000000000ULL),"present wait"))return -1;
    double completed=platform_time();
    g.wait_count=g.extra_count=0;g.signal=VK_NULL_HANDLE;
    if(g.present_external){if(!platform_gpu_scanout_select||!platform_gpu_scanout_select(pixels)){g.failed=true;return -1;}}
    else memcpy(platform_framebuffer(),pixels,1280*720*4);
    if(!g.first_frame){g.first_frame=true;nps64_log("[gpu] first scanout %ux%u GPU compositor fence completed\n",width,height);}
    if(g.scanout_width!=width||g.scanout_height!=height){g.scanout_width=width;g.scanout_height=height;nps64_log("[gpu] scanout geometry %ux%u RGBA format=%u\n",width,height,g.format);}
    gpu_perf.compositions++;gpu_perf.compose_acquire_seconds+=acquired-started;
    gpu_perf.compose_record_seconds+=recorded-acquired;gpu_perf.compose_submit_seconds+=submitted-recorded;
    gpu_perf.compose_wait_seconds+=completed-submitted;gpu_perf.compose_copy_seconds+=platform_time()-completed;
    g.present_index=(g.present_index+1)%3;return 1;
#endif
}
bool nps64_gpu_failed(void) {
#if defined(__ORBIS__) && defined(NPS64_GPU_GAME_TEST)
    return g.failed || nps64_pipeline_guard_failed();
#else
    return g.failed;
#endif
}
void nps64_gpu_destroy_context(void) {
    if(g.reset_called && g.hw.context_destroy){g.hw.context_destroy();g.reset_called=false;}
}
void nps64_gpu_close(void) {
    nps64_gpu_destroy_context();
    if(g.context.device && fn_DestroyDevice) {
        if(fn_DeviceWaitIdle)fn_DeviceWaitIdle(g.context.device);
#ifdef NPS64_GPU_PRESENT_TEST
        if(g.present_external&&platform_gpu_scanout_unregister)platform_gpu_scanout_unregister();
        nps64_presenter_destroy(g.presenter);g.presenter=NULL;
#endif
        free_buffer();
        if(g.fence)fn_DestroyFence(g.context.device,g.fence,NULL);
        if(g.pool)fn_DestroyCommandPool(g.context.device,g.pool,NULL);
        if(g.negotiation.destroy_device)g.negotiation.destroy_device();
        fn_DestroyDevice(g.context.device,NULL);
    }
    if(g.instance && fn_DestroyInstance)fn_DestroyInstance(g.instance,NULL);
#ifndef __ORBIS__
    if(g.loader)dlclose(g.loader);
#endif
    memset(&g,0,sizeof(g));
}
