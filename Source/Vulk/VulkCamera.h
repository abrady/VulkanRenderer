#pragma once

struct VulkCamera
{
    glm::vec3 eye = glm::vec3(0.4f, .85f, 2.4f);
    float yaw = 180.0f;
    float pitch = -15.0f;

    glm::mat3 getRotMat()
    {
        return glm::eulerAngleXY(glm::radians(pitch), glm::radians(yaw));
    }
    glm::vec3 getForwardVec()
    {
        return getRotMat()[2];
    }
    glm::vec3 getRightVec()
    {
        glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
        return getRotMat() * right;
    }
    glm::vec3 getUpVec()
    {
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        return getRotMat() * up;
    }
};