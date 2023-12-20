#include "VulkGeo.h"
#include "VulkMesh.h"

void makeQuad(float x, float y, float w, float h, float depth, VulkMesh &meshData) {
    Vertex v0;
    v0.pos = glm::vec3(x, y, depth);
    v0.normal = glm::vec3(0.0f, 0.0f, 1.0f);
    v0.texCoord = glm::vec2(0.0f, 0.0f);

    Vertex v1;
    v1.pos = glm::vec3(x + w, y, depth);
    v1.normal = glm::vec3(0.0f, 0.0f, 1.0f);
    v1.texCoord = glm::vec2(1.0f, 0.0f);

    Vertex v2;
    v2.pos = glm::vec3(x + w, y + h, depth);
    v2.normal = glm::vec3(0.0f, 0.0f, 1.0f);
    v2.texCoord = glm::vec2(1.0f, 1.0f);

    Vertex v3;
    v3.pos = glm::vec3(x, y + h, depth);
    v3.normal = glm::vec3(0.0f, 0.0f, 1.0f);
    v3.texCoord = glm::vec2(0.0f, 1.0f);

    meshData.vertices.push_back(v0);
    meshData.vertices.push_back(v1);
    meshData.vertices.push_back(v2);
    meshData.vertices.push_back(v3);

    meshData.indices.push_back(0);
    meshData.indices.push_back(1);
    meshData.indices.push_back(2);
    meshData.indices.push_back(0);
    meshData.indices.push_back(2);
    meshData.indices.push_back(3);
}

void makeQuad(float w, float h, VulkMesh &meshData) {
    makeQuad(-w / 2.0f, -h / 2.0f, w, h, 0.0f, meshData);
}

void makeCap(uint32_t numSlices, float y, float r, float dTheta, VulkMesh &meshData) {
    uint32_t baseIndex = (uint32_t)meshData.vertices.size();
    for(uint32_t i = 0; i <= numSlices; ++i) {
        Vertex vertex;
        float c = cosf(i * dTheta);
        float s = sinf(i * dTheta);
        vertex.pos = glm::vec3(r * c, y, r * s);
        vertex.texCoord = glm::vec2(c, s);
        vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        meshData.vertices.push_back(vertex);
    }
    for(uint32_t i = 0; i < numSlices; ++i) {
        meshData.indices.push_back(baseIndex + i);
        meshData.indices.push_back(baseIndex + i + 1);
        meshData.indices.push_back(baseIndex + numSlices + 1);
    }
}


void makeCylinder(float height, float bottomRadius, float topRadius, uint32_t numStacks, uint32_t numSlices, VulkMesh &meshData) {
    float stackHeight = height / numStacks;
    float radiusStep = (topRadius - bottomRadius) / numStacks;
    uint32_t ringCount = numStacks + 1;

    uint32_t baseIndex = (uint32_t)meshData.vertices.size();
    for(uint32_t i = 0; i < ringCount; ++i) {
        float y = -0.5f * height + i * stackHeight;
        float r = bottomRadius + i * radiusStep;
        float dTheta = 2.0f * glm::pi<float>() / numSlices;
        for(uint32_t j = 0; j <= numSlices; ++j) {
            Vertex vertex;
            float c = cosf(j * dTheta);
            float s = sinf(j * dTheta);
            vertex.pos = glm::vec3(r * c, y, r * s);
            vertex.texCoord = glm::vec2((float)j / numSlices, 1.0f - (float)i / numStacks);
            vertex.normal = glm::vec3(c, 0.0f, s);
            meshData.vertices.push_back(vertex);
        }
    }
    uint32_t ringVertexCount = numSlices + 1;
    for(uint32_t i = 0; i < numStacks; ++i) {
        for(uint32_t j = 0; j < numSlices; ++j) {
            meshData.indices.push_back(baseIndex + i * ringVertexCount + j);
            meshData.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
            meshData.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
            meshData.indices.push_back(baseIndex + i * ringVertexCount + j);
            meshData.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
            meshData.indices.push_back(baseIndex + i * ringVertexCount + j + 1);
        }
    }

    // cap vertices
    makeCap(numSlices, -0.5f * height, bottomRadius, glm::pi<float>() * 2.0f, meshData);
    makeCap(numSlices, 0.5f * height, topRadius, glm::pi<float>() * 2.0f, meshData);
}
