#include "VulkMesh.h"

void VulkMesh::createRender(Vulk &vk, char const *texturePath) {
    createVertexBuffer(vk);
    createIndexBuffer(vk);
    vk.createTextureImage(texturePath, textureImageMemory, textureImage);
    textureImageView = vk.createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    textureSampler = vk.createTextureSampler();
}

void VulkMesh::createVertexBuffer(Vulk &vk) {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    vk.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(vk.device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(vk.device, stagingBufferMemory);

    vk.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

    vk.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(vk.device, stagingBuffer, nullptr);
    vkFreeMemory(vk.device, stagingBufferMemory, nullptr);
}

void VulkMesh::createIndexBuffer(Vulk &vk) {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    vk.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(vk.device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    vkUnmapMemory(vk.device, stagingBufferMemory);

    vk.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

    vk.copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(vk.device, stagingBuffer, nullptr);
    vkFreeMemory(vk.device, stagingBufferMemory, nullptr);
}

VkDescriptorImageInfo VulkMesh::getTextureDescriptorImageInfo() {
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = textureImageView;
    imageInfo.sampler = textureSampler;
    return imageInfo;
}


void VulkMesh::cleanup(Vulk &vk) {
    vkDestroySampler(vk.device, textureSampler, nullptr);
    vkDestroyImageView(vk.device, textureImageView, nullptr);
    vkDestroyImage(vk.device, textureImage, nullptr);
    vkFreeMemory(vk.device, textureImageMemory, nullptr);
    vkDestroyBuffer(vk.device, indexBuffer, nullptr);
    vkFreeMemory(vk.device, indexBufferMemory, nullptr);
    vkDestroyBuffer(vk.device, vertexBuffer, nullptr);
    vkFreeMemory(vk.device, vertexBufferMemory, nullptr);
}