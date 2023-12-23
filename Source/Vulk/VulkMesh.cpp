#include "VulkMesh.h"

VulkMeshRef VulkMesh::appendMesh(VulkMesh const &mesh) {
    uint32_t vertexOffset = static_cast<uint32_t>(vertices.size());
    vertices.insert(vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
    uint32_t indexOffset = static_cast<uint32_t>(indices.size());
    indices.insert(indices.end(), mesh.indices.begin(), mesh.indices.end());
    // for (uint32_t index : mesh.indices) {
    //     indices.push_back(index + vertexOffset);
    // }
    
    return VulkMeshRef{mesh.name, vertexOffset, indexOffset, static_cast<uint32_t>(mesh.indices.size())};
}