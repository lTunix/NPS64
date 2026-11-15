/* Optional GPU compositor; owns outputs, never owns the core image. */
#pragma once
#include <vulkan/vulkan.h>
#include <stdbool.h>
#define NPS64_PRESENT_BUFFERS 3
typedef struct Nps64Presenter Nps64Presenter;
Nps64Presenter *nps64_presenter_create(VkPhysicalDevice,VkDevice,PFN_vkGetInstanceProcAddr,VkInstance,PFN_vkGetDeviceProcAddr);
void nps64_presenter_invalidate(Nps64Presenter*);
void nps64_presenter_destroy(Nps64Presenter*);
void *nps64_presenter_pixels(Nps64Presenter*,unsigned);
bool nps64_presenter_record(Nps64Presenter*,VkCommandBuffer,VkImageView,VkImageLayout,unsigned,unsigned,unsigned,bool,bool);
