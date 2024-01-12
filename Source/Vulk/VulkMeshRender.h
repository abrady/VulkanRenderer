#pragma once

#include "Vulk.h"

struct VulkMeshRender
{
    Vulk &vk;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkDescriptorSet descriptorSets[MAX_FRAMES_IN_FLIGHT];
    uint32_t numIndices;

    VulkMeshRender(Vulk &vk, VulkMesh const &mesh) : vk(vk)
    {
        auto &vertices = mesh.vertices;
        auto &indices = mesh.indices;
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        vk.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
        bufferSize = sizeof(indices[0]) * indices.size();
        vk.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

        bufferSize = sizeof(vertices[0]) * vertices.size();
        vk.copyFromMemToBuffer(vertices.data(), vertexBuffer, bufferSize);

        bufferSize = sizeof(indices[0]) * indices.size();
        vk.copyFromMemToBuffer(indices.data(), indexBuffer, bufferSize);

        numIndices = (uint32_t)indices.size();
    }

    ~VulkMeshRender()
    {
        vkDestroyBuffer(vk.device, vertexBuffer, nullptr);
        vkFreeMemory(vk.device, vertexBufferMemory, nullptr);
        vkDestroyBuffer(vk.device, indexBuffer, nullptr);
        vkFreeMemory(vk.device, indexBufferMemory, nullptr);
        vkDestroyDescriptorSetLayout(vk.device, descriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(vk.device, descriptorPool, nullptr);
        vkDestroyPipelineLayout(vk.device, pipelineLayout, nullptr);
        vkDestroyPipeline(vk.device, pipeline, nullptr);
    }
};