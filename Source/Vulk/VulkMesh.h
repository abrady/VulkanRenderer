#pragma once

#include "Vulk.h"

class VulkMesh {
public:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
