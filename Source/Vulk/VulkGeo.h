#pragma once

#include "VulkUtil.h"

class VulkMesh;

void makeQuad(float w, float h, uint32_t numSubdivisions, VulkMesh &meshData);
void makeQuad(float x, float y, float w, float h, float depth, uint32_t numSubdivisions, VulkMesh &meshData);

// make a cylinder of height `height` with radius `radius`, `numStacks` and `numSlices` slices (stacks are vertical, slices are horizontal - think pizza slices)
// vertices and indices are output parameters that are appended to
void makeCylinder(float height, float bottomRadius, float topRadius, uint32_t numStacks, uint32_t numSlices, VulkMesh &meshData);
void makeGeoSphere(float radius, uint32_t numSubdivisions, VulkMesh &meshData);
