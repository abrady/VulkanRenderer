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
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <array>
#include <optional>
#include <set>
#include <unordered_map>
#include <cassert>
#include <vulkan/vk_enum_string_helper.h>

#define VK_CALL(func) \
do { \
    VkResult result = (func); \
    if (result != VK_SUCCESS) { \
        std::cerr << "Vulkan error: " << string_VkResult(result) << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        throw std::runtime_error(std::string("Vulkan error: ") + string_VkResult(result) + " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
    } \
} while (0)


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

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, NumBindingLocations> getAttributeDescriptions();
};

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
void loadModel(char const *model_path, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices);
std::vector<char> readFileIntoMem(const std::string& filename);

