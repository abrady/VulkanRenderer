/*
 * This file defines a templated struct called VulkStorageBuffer, which represents a Vulkan storage buffer.
 *
 * Vulkan storage buffers (SSBOs - Shader Storage Buffer Objects) are a type of buffer in Vulkan that can be used to store data that is accessible by shaders.
 * They provide a way to share data between different stages of the graphics pipeline, such as between the vertex shader and the fragment shader.
 * SSBOs are useful when you need to pass large amounts of data to shaders, or when you need to read and write data from within a shader.
 *
 * The VulkStorageBuffer struct encapsulates a Vulkan buffer handle, a device memory handle, and a contiguous array of memory mapped objects.
 * It provides methods to create and map the memory for the buffer, as well as clean up the buffer and device memory.
 *
 * Usage example:
 *
 * VulkStorageBuffer<MyData> storageBuffer;
 * storageBuffer.createAndMap(vk, numObjects);
 *
 * // Access and modify the mapped objects
 * for (uint32_t i = 0; i < numObjects; i++) {
 *     storageBuffer.mappedObjs[i].data = ...;
 * }
 *
 * storageBuffer.cleanup(vk.device);
 */
#pragma once

#include "Vulk.h"
#include "Common/ClassNonCopyableNonMovable.h"

template <typename T>
class VulkStorageBuffer : public ClassNonCopyableNonMovable
{
public:
    VkBuffer buf;       // Vulkan buffer handle
    VkDeviceMemory mem; // Vulkan device memory handle
    T *mappedObjs;      // Contiguous array of memory mapped objects
    uint32_t numObjs;   // Number of objects in the buffer

    /**
     * Creates a Vulkan storage buffer and maps the memory for the specified number of objects.
     * @param vk The Vulk object used for Vulkan operations.
     * @param numActors The number of objects to be stored in the buffer.
     */
    void createAndMap(Vulk &vk, uint32_t numElts)
    {
        VkDeviceSize bufferSize = sizeof(T) * numElts;
        vk.createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buf, mem);
        vkMapMemory(vk.device, mem, 0, bufferSize, 0, (void **)&mappedObjs);
        numObjs = numElts;
    }

    /**
     * Cleans up the Vulkan buffer and device memory.
     * @param dev The Vulkan device.
     */
    void cleanup(VkDevice dev)
    {
        vkDestroyBuffer(dev, buf, nullptr);
        vkFreeMemory(dev, mem, nullptr);
    }

    uint32_t getSize()
    {
        return sizeof(T) * numObjs;
    }
};