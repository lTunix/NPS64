#pragma once
#include <vulkan/vulkan.h>
/* Synchronous handoff: inputs stay alive until the join completes. */
VkResult nps64_gpu_compile_worker(PFN_vkCreateComputePipelines,VkDevice,VkPipelineCache,
    uint32_t,const VkComputePipelineCreateInfo*,const VkAllocationCallbacks*,VkPipeline*);
