#pragma once

#include "VulkUtil.h"
#include "VulkUniformBuffer.h"

template <typename T>
class VulkFrameUBOs
{
    Vulk &vk;
    std::array<VulkUniformBuffer<T>, MAX_FRAMES_IN_FLIGHT> ubos;

public:
    explicit VulkFrameUBOs(Vulk &vk) : vk(vk)
    {
        for (auto &ubo : ubos)
        {
            ubo.init(vk);
        }
    }
    VulkFrameUBOs(Vulk &vk, T rhs) : vk(vk)
    {
        for (auto &ubo : ubos)
        {
            ubo.init(vk);
            *ubo.mappedUBO = rhs;
        }
    }
    ~VulkFrameUBOs()
    {
        for (auto &ubo : ubos)
        {
            ubo.cleanup(vk);
        }
    }
    std::array<VulkUniformBuffer<T>, MAX_FRAMES_IN_FLIGHT> &getUBOs()
    {
        return ubos;
    }
    std::array<VulkUniformBuffer<T>, MAX_FRAMES_IN_FLIGHT>::iterator begin()
    {
        return ubos.begin();
    }
    std::array<VulkUniformBuffer<T>, MAX_FRAMES_IN_FLIGHT>::iterator end()
    {
        return ubos.end();
    }
    T *operator[](uint32_t frame)
    {
        return ubos[frame].mappedUBO;
    }
};