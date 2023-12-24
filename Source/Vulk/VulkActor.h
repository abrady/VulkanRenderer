#pragma once

#include "Vulk.h"
#include "VulkMesh.h"

class Scene;

class VulkActor {
public:
    char const *name = nullptr;
    glm::mat4 xform = glm::mat4(1.0f);
};