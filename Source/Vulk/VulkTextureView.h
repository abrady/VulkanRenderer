#pragma once
#include <vulkan/vulkan.h>

class Vulk;
struct VulkTextureView
{
    Vulk &vk;
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;

    VulkTextureView(Vulk &vkIn, char const *texturePath);
    VulkTextureView(Vulk &vkIn, std::string const &texturePath) : VulkTextureView(vkIn, texturePath.c_str()) {}
    ~VulkTextureView();
};
