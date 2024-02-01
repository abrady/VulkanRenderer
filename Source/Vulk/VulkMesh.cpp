#include "VulkMesh.h"

VulkMeshRef VulkMesh::appendMesh(VulkMesh const &mesh)
{
    uint32_t vertexOffset = static_cast<uint32_t>(vertices.size());
    vertices.insert(vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
    uint32_t indexOffset = static_cast<uint32_t>(indices.size());
    for (uint32_t index : mesh.indices)
    {
        indices.push_back(index + vertexOffset);
    }

    return VulkMeshRef{mesh.name, vertexOffset, indexOffset, static_cast<uint32_t>(mesh.indices.size())};
}

void VulkMesh::xform(glm::mat4 const &xform)
{
    for (Vertex &v : vertices)
    {
        v.pos = glm::vec3(xform * glm::vec4(v.pos, 1.0f));
        v.normal = glm::vec3(xform * glm::vec4(v.normal, 0.0f));
        v.tangent = glm::vec3(xform * glm::vec4(v.tangent, 0.0f));
    }
}

VulkMesh VulkMesh::loadFromFile(char const *filename)
{
    VulkMesh model;
    loadModel(filename, model.vertices, model.indices);
    assert(model.vertices.size() > 0);
    assert(model.indices.size() > 0);
    return model;
}
