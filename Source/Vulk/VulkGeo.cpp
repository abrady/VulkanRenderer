#include "VulkGeo.h"
#include "VulkMesh.h"

// TODO: get subdivideTris working with our geo creation functions and maybe we can accumulate the vertices and indices in the meshData
#define CHECK_MESH_DATA(meshData) assert(meshData.vertices.size() == 0 && meshData.indices.size() == 0)

// Turn a single triangle into 4 triangles by adding 3 new vertices at the midpoints of each edge
// Original Triangle:
//      /\
//     /  \
//    /    \
//   /______\
//
// Subdivision Step:
//      /\
//     /__\
//    /\  /\
//   /__\/__\
//
static void subdivideTris(VulkMesh &meshData) {
    // save a copy of the input geometry
    std::vector<Vertex> verticesCopy = meshData.vertices;
    std::vector<uint32_t> indicesCopy = meshData.indices;
    meshData.vertices.clear();
    meshData.indices.clear();
    meshData.vertices.reserve(verticesCopy.size() * 2);
    meshData.indices.reserve(indicesCopy.size() * 4);
    for (uint32_t i = 0; i < indicesCopy.size(); i += 3) {
        Vertex v0 = verticesCopy[indicesCopy[i + 0]];
        Vertex v1 = verticesCopy[indicesCopy[i + 1]];
        Vertex v2 = verticesCopy[indicesCopy[i + 2]];

        // generate 3 new vertices at the midpoints of each edge
        Vertex m0, m1, m2;
        m0.pos = 0.5f * (v0.pos + v1.pos);
        m0.texCoord = 0.5f * (v0.texCoord + v1.texCoord);
        m1.pos = 0.5f * (v1.pos + v2.pos);
        m1.texCoord = 0.5f * (v1.texCoord + v2.texCoord);
        m2.pos = 0.5f * (v0.pos + v2.pos);
        m2.texCoord = 0.5f * (v0.texCoord + v2.texCoord);


        // add new geometry
        meshData.vertices.push_back(v0); 
        meshData.vertices.push_back(m0);
        meshData.vertices.push_back(v1);
        meshData.vertices.push_back(m1);
        meshData.vertices.push_back(v2);
        meshData.vertices.push_back(m2);

        // add new indices
        uint32_t numVertices = (uint32_t)meshData.vertices.size() - 6;
        meshData.indices.push_back(numVertices + 0);
        meshData.indices.push_back(numVertices + 1);
        meshData.indices.push_back(numVertices + 5);

        meshData.indices.push_back(numVertices + 1);
        meshData.indices.push_back(numVertices + 2);
        meshData.indices.push_back(numVertices + 3);

        meshData.indices.push_back(numVertices + 1);
        meshData.indices.push_back(numVertices + 3);
        meshData.indices.push_back(numVertices + 5);

        meshData.indices.push_back(numVertices + 3);
        meshData.indices.push_back(numVertices + 4);
        meshData.indices.push_back(numVertices + 5);
    }
}

void makeEquilateralTri(float side, uint32_t numSubdivisions, VulkMesh &meshData) {
    CHECK_MESH_DATA(meshData);
    meshData.name = "EquilateralTriangle";
    Vertex v0;
    v0.pos = glm::vec3(0.0f, 0.0f, 0.0f);
    v0.normal = glm::vec3(0.0f, 0.0f, 1.0f);
    v0.texCoord = glm::vec2(0.5f, 0.0f);

    Vertex v1;
    v1.pos = glm::vec3(side, 0.0f, 0.0f);
    v1.normal = glm::vec3(0.0f, 0.0f, 1.0f);
    v1.texCoord = glm::vec2(1.0f, 1.0f);

    Vertex v2;
    v2.pos = glm::vec3(side / 2.0f, side * sqrtf(3.0f) / 2.0f, 0.0f);
    v2.normal = glm::vec3(0.0f, 0.0f, 1.0f);
    v2.texCoord = glm::vec2(0.0f, 1.0f);

    v0.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
    v1.tangent = glm::normalize(v2.pos - v1.pos);
    v2.tangent = glm::normalize(v0.pos - v2.pos);

    uint32_t baseIndex = (uint32_t)meshData.vertices.size();
    meshData.vertices.push_back(v0);
    meshData.vertices.push_back(v1);
    meshData.vertices.push_back(v2);

    meshData.indices.push_back(baseIndex + 0);
    meshData.indices.push_back(baseIndex + 1);
    meshData.indices.push_back(baseIndex + 2);

    assert(numSubdivisions <= 6u);
    numSubdivisions = glm::min(numSubdivisions, 6u);
    for(uint32_t i = 0; i < numSubdivisions; ++i) {
        subdivideTris(meshData);
    }
}

void makeQuad(float x, float y, float w, float h, float depth, uint32_t numSubdivisions, VulkMesh &meshData) {
    CHECK_MESH_DATA(meshData);
    meshData.name = "Quad";

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

    uint32_t baseIndex = (uint32_t)meshData.vertices.size();
    meshData.vertices.push_back(v0);
    meshData.vertices.push_back(v1);
    meshData.vertices.push_back(v2);
    meshData.vertices.push_back(v3);

    meshData.indices.push_back(baseIndex + 0);
    meshData.indices.push_back(baseIndex + 1);
    meshData.indices.push_back(baseIndex + 2);
    meshData.indices.push_back(baseIndex + 0);
    meshData.indices.push_back(baseIndex + 2);
    meshData.indices.push_back(baseIndex + 3);

    assert(numSubdivisions <= 6u);
    numSubdivisions = glm::min(numSubdivisions, 6u);
    for(uint32_t i = 0; i < numSubdivisions; ++i) {
        subdivideTris(meshData);
    }
}

void makeQuad(float w, float h, uint32_t numSubdivisions, VulkMesh &meshData) {
    makeQuad(-w / 2.0f, -h / 2.0f, w, h, 0.0f, numSubdivisions, meshData);
}

// make a cylinder of height `height` with radius `radius`, `numStacks` and `numSlices` slices (stacks are vertical, slices are horizontal - think pizza slices)
// cenetered at the origin
void makeCylinder(float height, float bottomRadius, float topRadius, uint32_t numStacks, uint32_t numSlices, VulkMesh &meshData) {
    CHECK_MESH_DATA(meshData);
    meshData.name = "Cylinder";

    uint32_t baseIndex;

    float stackHeight = height / numStacks;
    float radiusStep = (topRadius - bottomRadius) / numStacks;
    uint32_t ringCount = numStacks + 1;
    baseIndex = (uint32_t)meshData.vertices.size();
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

    float const dTheta = 2.0f * glm::pi<float>() / numSlices;
    uint32_t centerIndex;
    
    // // make top
    baseIndex = (uint32_t)meshData.vertices.size();
    for(uint32_t i = 0; i <= numSlices; ++i) {
        Vertex vertex;
        float c = cosf(i * dTheta);
        float s = sinf(i * dTheta);
        float x = topRadius * c;
        float z = topRadius * s;
        float u = x / height + 0.5f;
        float v = z / height + 0.5f;
        vertex.pos = glm::vec3(x, 0.5f * height, z);
        vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        vertex.tangent = glm::vec3(-s, 0.0f, c);
        vertex.texCoord = glm::vec2(u, v);
        meshData.vertices.push_back(vertex);
    }
    Vertex topCenter;
    topCenter.pos = glm::vec3(0.0f, 0.5f * height, 0.0f);
    topCenter.texCoord = glm::vec2(0.5f, 0.5f);
    topCenter.normal = glm::vec3(0.0f, -1.0f, 0.0f);
    topCenter.tangent = glm::vec3(1.0f, 0.0f, 0.0f); // no idea on tangent for the topCenter point
    meshData.vertices.push_back(topCenter);
    centerIndex = (uint32_t)meshData.vertices.size() - 1;

    for(uint32_t i = 0; i < numSlices; ++i) {
        meshData.indices.push_back(baseIndex + i + 1);
        meshData.indices.push_back(baseIndex + i);
        meshData.indices.push_back(centerIndex);
    }

    // make bottom
    baseIndex = (uint32_t)meshData.vertices.size();
    for(uint32_t i = 0; i <= numSlices; ++i) {
        Vertex vertex;
        float c = cosf(i * dTheta);
        float s = sinf(i * dTheta);
        float x = bottomRadius * c;
        float z = bottomRadius * s;
        float u = x / height + 0.5f;
        float v = z / height + 0.5f;

        vertex.pos = glm::vec3(x, -0.5f * height, z);
        vertex.normal = glm::vec3(0.0f, -1.0f, 0.0f);
        vertex.tangent = glm::vec3(-s, 0.0f, c);
        vertex.texCoord = glm::vec2(u, v);
        meshData.vertices.push_back(vertex);
    }
    Vertex bottomCenter;
    bottomCenter.pos = glm::vec3(0.0f, -0.5f * height, 0.0f);
    bottomCenter.texCoord = glm::vec2(0.5f, 0.5f);
    bottomCenter.normal = glm::vec3(0.0f, -1.0f, 0.0f);
    bottomCenter.tangent = glm::vec3(1.0f, 0.0f, 0.0f); // no idea on tangent for the bottomCenter point
    meshData.vertices.push_back(bottomCenter);
    centerIndex = (uint32_t)meshData.vertices.size() - 1;

    for(uint32_t i = 0; i < numSlices; ++i) {
        meshData.indices.push_back(baseIndex + i);
        meshData.indices.push_back(baseIndex + i + 1);
        meshData.indices.push_back(centerIndex);
    }
}


void makeGeoSphere(float radius, uint32_t numSubdivisions, VulkMesh &meshData) {
    CHECK_MESH_DATA(meshData);
    meshData.name = "GeoSphere";

    // put a cap on the number of subdivisions
    numSubdivisions = glm::min(numSubdivisions, 6u);

    // put a cap on the number of subdivisions
    const float x = 0.525731f;
    const float z = 0.850651f;

    glm::vec3 pos[12] = {
        glm::vec3(-x, 0.0f, z), glm::vec3(x, 0.0f, z),
        glm::vec3(-x, 0.0f, -z), glm::vec3(x, 0.0f, -z),
        glm::vec3(0.0f, z, x), glm::vec3(0.0f, z, -x),
        glm::vec3(0.0f, -z, x), glm::vec3(0.0f, -z, -x),
        glm::vec3(z, x, 0.0f), glm::vec3(-z, x, 0.0f),
        glm::vec3(z, -x, 0.0f), glm::vec3(-z, -x, 0.0f)
    };

    uint32_t k[60] = {
        1, 4, 0, 4, 9, 0, 4, 5, 9, 8, 5, 4,
        1, 8, 4, 1, 10, 8, 10, 3, 8, 8, 3, 5,
        3, 2, 5, 3, 7, 2, 3, 10, 7, 10, 6, 7,
        6, 11, 7, 6, 0, 11, 6, 1, 0, 10, 1, 6,
        11, 0, 9, 2, 11, 9, 5, 2, 9, 11, 2, 7
    };

    uint32_t baseIndex = (uint32_t)meshData.vertices.size();
    meshData.vertices.resize(meshData.vertices.size() + 12);
    meshData.indices.resize(meshData.indices.size() + 60);
    for(uint32_t i = 0; i < 60; ++i) {
        meshData.indices[i] = baseIndex + k[i];
    }

    for(uint32_t i = 0; i < 12; ++i) {
        meshData.vertices[i].pos = pos[i];
    }

    for(uint32_t i = 0; i < numSubdivisions; ++i) {
        subdivideTris(meshData);
    }

    // project vertices onto sphere and scale
    for(uint32_t i = 0; i < meshData.vertices.size(); ++i) {
        glm::vec3 n = glm::normalize(meshData.vertices[i].pos);
        glm::vec3 p = radius * n;
        meshData.vertices[i].pos = p;
        meshData.vertices[i].normal = n;
        meshData.vertices[i].texCoord.x = atan2f(n.z, n.x) / (2.0f * glm::pi<float>()) + 0.5f;
        meshData.vertices[i].texCoord.y = asinf(n.y) / glm::pi<float>() + 0.5f;
        meshData.vertices[i].tangent = glm::vec3(1.0f, 0.0f, 0.0f); // TODO: calculate tangent
    }
}

void makeAxes(float length, VulkMesh &meshData) {
    CHECK_MESH_DATA(meshData);
    meshData.name = "Axes";
    VulkMesh x,y,z;
    makeCylinder(length, 0.01f, 0.01f, 10, 10, x);
    makeCylinder(length, 0.01f, 0.01f, 10, 10, y);
    makeCylinder(length, 0.01f, 0.01f, 10, 10, z);

    glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), glm::pi<float>() / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 rotAndTranslateX = glm::translate(glm::mat4(1.0f), glm::vec3(length / 2.0f, 0.0f, 0.0f)) * rotX;
    x.xform(rotAndTranslateX);

    glm::mat4 transY = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, length / 2.0f, 0.0f));
    y.xform(transY);

    glm::mat4 rotZ = glm::rotate(glm::mat4(1.0f), glm::pi<float>() / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotAndTranslateZ = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, length / 2.0f)) * rotZ;
    z.xform(rotAndTranslateZ);


    meshData.appendMesh(x);
    meshData.appendMesh(y);
    meshData.appendMesh(z);
}

void makeGrid(float width, float depth, uint32_t m, uint32_t n, VulkMesh &meshData, float repeatU, float repeatV) {
    uint32_t vertexCount = m*n;
    uint32_t faceCount = (m-1)*(n-1)*2;

    // Create the vertices
    float halfWidth = 0.5f*width;
    float halfDepth = 0.5f*depth;

    float dx = width / (n-1);
    float dz = depth / (m-1);

    float du = repeatU / (n-1);
    float dv = repeatV / (m-1);

    meshData.vertices.resize(vertexCount);
    for(uint32_t i = 0; i < m; ++i)
    {
        float z = halfDepth - i*dz;
        for(uint32_t j = 0; j < n; ++j)
        {
            float x = -halfWidth + j*dx;

            meshData.vertices[i*n+j].pos = glm::vec3(x, 0.0f, z);
            meshData.vertices[i*n+j].normal = glm::vec3(0.0f, 1.0f, 0.0f);
            meshData.vertices[i*n+j].tangent = glm::vec3(1.0f, 0.0f, 0.0f);
            meshData.vertices[i*n+j].texCoord = glm::vec2(j*du, i*dv);
        }
    }

    // Create the indices.
    meshData.indices.resize(faceCount*3); // 3 indices per face

    // Iterate over each quad and compute indices.
    uint32_t k = 0;
    for(uint32_t i = 0; i < m-1; ++i)
    {
        for(uint32_t j = 0; j < n-1; ++j)
        {
            meshData.indices[k]   = i*n+j;
            meshData.indices[k+1] = i*n+j+1;
            meshData.indices[k+2] = (i+1)*n+j;

            meshData.indices[k+3] = (i+1)*n+j;
            meshData.indices[k+4] = i*n+j+1;
            meshData.indices[k+5] = (i+1)*n+j+1;

            k += 6; // next quad
        }
    }
}