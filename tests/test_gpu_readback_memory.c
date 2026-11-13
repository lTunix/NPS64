#include "gpu_readback_memory.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    VkPhysicalDeviceMemoryProperties p={0};p.memoryTypeCount=4;
    p.memoryTypes[0].propertyFlags=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    p.memoryTypes[1].propertyFlags=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    p.memoryTypes[2].propertyFlags=p.memoryTypes[1].propertyFlags|VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    p.memoryTypes[3].propertyFlags=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    assert(nps64_readback_memory_type(&p,15)==2);
    assert(nps64_readback_memory_type(&p,11)==3);
    assert(nps64_readback_memory_type(&p,3)==1);
    assert(nps64_readback_memory_type(&p,1)==UINT32_MAX);
    assert(nps64_readback_memory_type(&p,0)==UINT32_MAX);
    p.memoryTypes[3].propertyFlags=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    assert(nps64_readback_memory_type(&p,8)==3);
    p.memoryTypeCount=32;p.memoryTypes[31].propertyFlags=p.memoryTypes[2].propertyFlags;
    assert(nps64_readback_memory_type(&p,UINT32_C(0x80000000))==31);
    puts("PASS readback memory: CPU cache preference, coherent/noncoherent fallback, type mask, unavailable memory and bit31");
}
