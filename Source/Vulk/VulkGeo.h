/***************************************************************************
 *  Description:  Geometry generation functions
 * All generated geo is
 * 1. centered at (0,0) and 
 * 2. tends to be on the x/y plane where applicable
 ***************************************************************************/
#pragma once

#include "VulkUtil.h"

class VulkMesh;

void makeEquilateralTri(float side, uint32_t numSubdivisions, VulkMesh &meshData);

// make a quad in the x/y centered at 0,0
void makeQuad(float w, float h, uint32_t numSubdivisions, VulkMesh &meshData);
void makeQuad(float x, float y, float w, float h, float depth, uint32_t numSubdivisions, VulkMesh &meshData);

// make a cylinder of height `height` with radius `radius`, `numStacks` and `numSlices` slices (stacks are vertical, slices are horizontal - think pizza slices)
// vertices and indices are output parameters that are appended to
void makeCylinder(float height, float bottomRadius, float topRadius, uint32_t numStacks, uint32_t numSlices, VulkMesh &meshData);
void makeGeoSphere(float radius, uint32_t numSubdivisions, VulkMesh &meshData);
void makeAxes(float length, VulkMesh &meshData);

void makeGrid(float width, float depth, uint32_t m, uint32_t n, VulkMesh &meshData, float repeatU = 1.0f, float repeatV = 1.0f);