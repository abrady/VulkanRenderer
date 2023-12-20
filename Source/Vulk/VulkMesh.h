#pragma once

#include "Vulk.h"

class VulkMesh {
public:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    void createRender(Vulk &vk, char const *texturePath);
    VkDescriptorImageInfo getTextureDescriptorImageInfo();

    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;
    void cleanup(Vulk &vk);
private:
    VkDeviceMemory vertexBufferMemory;
    VkDeviceMemory indexBufferMemory;
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    void createVertexBuffer(Vulk &vk);
    void createIndexBuffer(Vulk &vk);
};
