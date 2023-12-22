#include "VulkMesh.h"

VulkMeshRef VulkMesh::appendMesh(VulkMesh const &mesh) {
    uint32_t indexOffset = static_cast<uint32_t>(vertices.size());
    vertices.insert(vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
    for (uint32_t index : mesh.indices) {
        indices.push_back(index + indexOffset);
    }

    return VulkMeshRef{mesh.name, indexOffset, static_cast<uint32_t>(mesh.indices.size())};
}
