#pragma once

#include "Vulk.h"

struct VulkMeshRef {
        char const *name = nullptr;
        uint32_t firstIndex = 0;
        uint32_t indexCount = 0;
};

class VulkMesh {
public:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
