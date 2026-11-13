#pragma once
#include <vulkan/vulkan.h>
/* Single-device diagnostic. Logs entry and return without changing arguments. */
PFN_vkGetInstanceProcAddr nps64_gpu_api_trace_wrap(PFN_vkGetInstanceProcAddr);
