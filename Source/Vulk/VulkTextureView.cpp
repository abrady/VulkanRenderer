#include "Vulk.h"
#include "VulkTextureView.h"

VulkTextureView::VulkTextureView(Vulk &vkIn, char const *texturePath): vk(vkIn) {
    vk.createTextureImage(texturePath, textureImageMemory, textureImage);
    textureImageView = vk.createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

VulkTextureView::~VulkTextureView() {
    vkDestroyImageView(vk.device, textureImageView, nullptr);
    vkDestroyImage(vk.device, textureImage, nullptr);
    vkFreeMemory(vk.device, textureImageMemory, nullptr);
}
