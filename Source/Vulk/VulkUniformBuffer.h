/***
 * @brief Uniform Buffer Object
 *
 * The VulkUniformBuffer class represents a uniform buffer object in Vulkan.
 *
 * Uniform buffers are a type of buffer object used in Vulkan to store uniform data
 * that is shared between the CPU and the GPU. They are typically used to pass
 * per-object or per-frame data to shaders, such as transformation matrices or
 * material properties.
 *
 * This template class allows you to create and manage uniform buffers of a specific
 * type T. The init method is used to create the Vulkan buffer and
 * allocate memory for the uniform buffer object. The mappedUBO member variable
 * provides a pointer to the mapped memory of the uniform buffer, allowing you to
 * update its contents.
 *
 * Note that uniform memory is typically limited and you should familarize yourself
 * with how much you have availabe on the device you are using. You can query the
 * amount of uniform memory available on a device by calling the getUniformBufferOffsetAlignment
 *
 * Example usage:
 *
 * VulkUniformBuffer<UniformBufferObject> ubo;
 * ubo.init(vk);
 *
 * // Update the contents of the uniform buffer
 * ubo.mappedUBO->model = glm::mat4(1.0f);
 * ubo.mappedUBO->view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
 * ubo.mappedUBO->projection = glm::perspective(glm::radians(45.0f), swapchainExtent.width / (float)swapchainExtent.height, 0.1f, 10.0f);
 *
 * // ...
 *
 * // Clean up the uniform buffer
 * ubo.cleanup(vk);
 */
#pragma once

#include <vulkan/vulkan.h>
#include "Vulk.h"
#include "Common/ClassNonCopyableNonMovable.h"

template <typename T>
class VulkUniformBuffer : public ClassNonCopyableNonMovable
{
    VkDeviceMemory mem;

public:
    VkBuffer buf;
    T *mappedUBO;

    void init(Vulk &vk)
    {
        VkDeviceSize bufferSize = sizeof(T);
        vk.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buf, mem);
        vkMapMemory(vk.device, mem, 0, bufferSize, 0, (void **)&mappedUBO);
    }

    void cleanup(Vulk &vk)
    {
        vkUnmapMemory(vk.device, mem);
        vkDestroyBuffer(vk.device, buf, nullptr);
        vkFreeMemory(vk.device, mem, nullptr);
    }

    VkDeviceSize getSize() const { return sizeof(T); }
};