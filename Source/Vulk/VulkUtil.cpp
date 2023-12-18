
// The following line is needed to make the implementation file for the header file
// i.e. the body of this functions will be defined in this file while other uses of the header
// will just declare the functions

#include "VulkUtil.h"

#pragma warning(push, 0) // assume these headers know what they're doing

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#pragma warning(pop)




std::array<VkVertexInputAttributeDescription, Vertex::NumBindingLocations> Vertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, Vertex::NumBindingLocations> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = Vertex::PosBinding;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = Vertex::ColorBinding;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = Vertex::NormalBinding;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, normal);

    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = Vertex::TangentBinding;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(Vertex, tangent);

    attributeDescriptions[4].binding = 0;
    attributeDescriptions[4].location = Vertex::TexCoordBinding;
    attributeDescriptions[4].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[4].offset = offsetof(Vertex, texCoord);

    return attributeDescriptions;
}