#pragma once
#include <vulkan/vulkan.h>

#include "Common/ClassNonCopyableNonMovable.h"

class Vulk;
struct VulkTextureView : public ClassNonCopyableNonMovable
{
    Vulk &vk;
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;

    VulkTextureView(Vulk &vkIn, std::filesystem::path const &texturePath);
    VulkTextureView(Vulk &vkIn, char const *texturePath);
    VulkTextureView(Vulk &vkIn, std::string const &texturePath) : VulkTextureView(vkIn, texturePath.c_str()) {}
    ~VulkTextureView();
};
