#pragma once

#include "VulkUtil.h"

class VulkMesh;

void makeQuad(float w, float h, VulkMesh &meshData);
void makeQuad(float x, float y, float w, float h, float depth, VulkMesh &meshData);

// make a cylinder of height `height` with radius `radius`, `numStacks` and `numSlices` slices (stacks are vertical, slices are horizontal - think pizza slices)
// vertices and indices are output parameters that are appended to
void makeCylinder(float height, float bottomRadius, float topRadius, uint32_t numStacks, uint32_t numSlices, VulkMesh &meshData);

// TODO: the normals make this more complicated than I want to think about right now, let's do this later
// make a cube of side length `side` with two triangles per face
// vertices and indices are output parameters that are appended to
inline void makeCubeNOTWORKING(uint32_t side, std::vector<Vertex> &vertices, std::vector<uint32_t> &/*indices*/) {
    // make a cube of side length `side` with two triangles per face
    // vertices and indices are output parameters that are appended to
    float half_side = side / 2.0f;
    float positions[] = {
        // front
        -half_side, -half_side, half_side,
        half_side, -half_side, half_side,
        half_side, half_side, half_side,
        -half_side, half_side, half_side,
        // back
        -half_side, -half_side, -half_side,
        half_side, -half_side, -half_side,
        half_side, half_side, -half_side,
        -half_side, half_side, -half_side,
    };
    float normals[] = {
        // front
        0, 0, 1,
        0, 0, 1,
        0, 0, 1,
        0, 0, 1,
        // back
        0, 0, -1,
        0, 0, -1,
        0, 0, -1,
        0, 0, -1
        // left
        -1, 0, 0,
        -1, 0, 0,
        -1, 0, 0,
        -1, 0, 0,
        // right
        1, 0, 0,
        1, 0, 0,
        1, 0, 0,
        1, 0, 0,
        // top
        0, 1, 0,
        0, 1, 0,
        0, 1, 0,
        0, 1, 0,
        // bottom
        0, -1, 0,
        0, -1, 0,
        0, -1, 0,
        0, -1, 0,
    };
    float texcoords[] = {
        // front
        0, 0,
        1, 0,
        1, 1,
        0, 1,
        // back
        0, 0,
        1, 0,
        1, 1,
        0, 1,
        // left
        0, 0,
        1, 0,
        1, 1,
        0, 1,
        // right
        0, 0,
        1, 0,
        1, 1,
        0, 1,
        // top
        0, 0,
        1, 0,
        1, 1,
        0, 1,
        // bottom
        0, 0,
        1, 0,
        1, 1,
        0, 1,
    };
    uint32_t indices_[] = {
        // front
        0, 1, 2,
        2, 3, 0,
        // right
        1, 5, 6,
        6, 2, 1,
        // back
        7, 6, 5,
        5, 4, 7,
        // left
        4, 0, 3,
        3, 7, 4,
        // bottom
        4, 5, 1,
        1, 0, 4,
        // top
        3, 2, 6,
        6, 7, 3
    };
    // size_t starting_index = indices.size();
    for (int i = 0; i < 8; i++) {
        Vertex v;
        v.pos = glm::vec3(positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]);
        v.normal = glm::vec3(normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]);
        v.texCoord = glm::vec2(texcoords[i * 2], texcoords[i * 2 + 1]);
        vertices.push_back(v);
    }
}

