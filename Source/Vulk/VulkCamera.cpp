#include "VulkCamera.h"

void VulkCamera::lookAt(glm::vec3 eyeIn, glm::vec3 target) {
    this->eye = eyeIn;
    glm::vec3 dir = glm::normalize(target - eye);
    glm::vec3 forward = glm::vec3(dir.x, 0.0f, dir.z);
    pitch = glm::degrees(glm::asin(dir.y));
    yaw = glm::degrees(atan2(forward.x, forward.z));
}

glm::mat3 VulkCamera::getRotMat()
{
    return glm::eulerAngleXY(glm::radians(pitch), glm::radians(yaw));
}
glm::vec3 VulkCamera::getForwardVec()
{
    return getRotMat()[2];
}
glm::vec3 VulkCamera::getRightVec()
{
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
    return getRotMat() * right;
}
glm::vec3 VulkCamera::getUpVec()
{
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    return getRotMat() * up;
}