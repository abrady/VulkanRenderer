#pragma once

#include "VulkUtil.h"

struct VulkCamera
{
    glm::vec3 eye = glm::vec3(0.4f, .85f, 2.4f);
    float yaw = 180.0f;
    float pitch = -15.0f;

    void lookAt(glm::vec3 eyeIn, glm::vec3 target);
    glm::mat3 getRotMat();
    glm::vec3 getForwardVec();
    glm::vec3 getRightVec();
    glm::vec3 getUpVec();
};