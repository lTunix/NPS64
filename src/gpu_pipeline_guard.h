#pragma once
#include <vulkan/vulkan.h>
/* Diagnostic guard: wraps device/pipeline creation; never submits commands. */
PFN_vkGetInstanceProcAddr nps64_pipeline_guard_wrap(PFN_vkGetInstanceProcAddr);
int nps64_pipeline_guard_failed(void);
