#pragma once
#include <vulkan/vulkan.h>
#include <stdint.h>
/* A transfer destination is read by the CPU. Prefer cached memory, preserving
 * compatibility with the buffer's memoryTypeBits and noncoherent fallback. */
static inline unsigned nps64_readback_memory_type(const VkPhysicalDeviceMemoryProperties *properties,uint32_t bits) {
    unsigned best=UINT32_MAX,score=0;
    for(unsigned i=0;i<properties->memoryTypeCount && i<32;i++) {
        VkMemoryPropertyFlags flags=properties->memoryTypes[i].propertyFlags;
        if(!(bits&(1u<<i)) || !(flags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))continue;
        unsigned rank=1u+((flags&VK_MEMORY_PROPERTY_HOST_CACHED_BIT)?4u:0u)+
            ((flags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)?2u:0u);
        if(rank>score){score=rank;best=i;}
    }
    return best;
}
