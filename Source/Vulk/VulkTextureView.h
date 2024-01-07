#pragma once
#include <vulkan/vulkan.h>

class Vulk; 
struct VulkTextureView {
    Vulk &vk;
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;

    VulkTextureView(Vulk &vkIn, char const *texturePath);
    ~VulkTextureView();
};
