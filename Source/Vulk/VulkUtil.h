#pragma once

#pragma warning(push, 0) // assume these headers know what they're doing

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <stb_image.h>
#include <tiny_obj_loader.h>

#pragma warning(pop)

#include <vector>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <array>
#include <optional>
#include <set>
#include <unordered_map>
#include <cassert>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include "VulkDescriptorSetLayoutBuilder.h"

#define VK_CALL(func) \
do { \
    VkResult vkcall_macro_result = (func); \
    if (vkcall_macro_result != VK_SUCCESS) { \
        std::cerr << "Vulkan error: " << string_VkResult(vkcall_macro_result) << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        throw std::runtime_error(std::string("Vulkan error: ") + string_VkResult(vkcall_macro_result) + " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
    } \
} while (0)

// keep in sync with Source\Shaders\Common\common.glsl
enum VulkShaderBindings {
    VulkShaderBinding_XformsUBO = 0,
    VulkShaderBinding_TextureSampler = 1,
    VulkShaderBinding_Actors = 2,
    VulkShaderBinding_Lights = 3,
    VulkShaderBinding_Materials = 4,
    VulkShaderBinding_EyePos = 5,
    VulkShaderBinding_TextureSampler2 = 6,
    VulkShaderBinding_TextureSampler3 = 7,
    VulkShaderBinding_WavesXform = 8,
    VulkShaderBinding_MaxBindingID,
};


struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 texCoord;

    enum BindingLocations {
        PosBinding = 0,
        NormalBinding = 1,
        TangentBinding = 2,
        TexCoordBinding = 3,
        NumBindingLocations = 4,
    };
};

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
void loadModel(char const *model_path, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices);
std::vector<char> readFileIntoMem(const std::string& filename);

