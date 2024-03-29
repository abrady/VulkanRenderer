#pragma once

#include "Vulk.h"

struct VulkMeshRef
{
    char const *name = nullptr;
    uint32_t firstVertex = 0;
    uint32_t firstIndex = 0;
    uint32_t indexCount = 0;
};

class VulkMesh
{
public:
    char const *name = nullptr;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    VulkMeshRef appendMesh(VulkMesh const &mesh);
    VulkMeshRef getMeshRef() const { return {name, 0, 0, (uint32_t)indices.size()}; };
    void xform(glm::mat4 const &xform);

    static VulkMesh loadFromFile(char const *filename);
    static VulkMesh loadFromPath(std::filesystem::path const &path) { return loadFromFile(path.string().c_str()); }
};
