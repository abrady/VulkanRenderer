#pragma once

#include "Vulk/Vulk.h"
#include "Vulk/VulkGeo.h"
#include "Vulk/VulkActor.h"
#include "Vulk/VulkCamera.h"
#include "Vulk/VulkPipelineBuilder.h"
#include "Vulk/VulkDescriptorPoolBuilder.h"
#include "Vulk/VulkUniformBuffer.h"
#include "Vulk/VulkStorageBuffer.h"
#include "Vulk/VulkDescriptorSetUpdater.h"

#pragma once

#include "Vulk/Vulk.h"
#include "Vulk/VulkGeo.h"
#include "Vulk/VulkActor.h"
#include "Vulk/VulkCamera.h"
#include "Vulk/VulkPipelineBuilder.h"
#include "Vulk/VulkDescriptorPoolBuilder.h"
#include "Vulk/VulkUniformBuffer.h"
#include "Vulk/VulkStorageBuffer.h"
#include "Vulk/VulkDescriptorSetUpdater.h"
#include "Vulk/VulkTextureView.h"

class BlendingWorld
{
public:
    Vulk &vk;
    VulkCamera camera;

    struct XformsUBO
    {
        alignas(16) glm::mat4 world;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };
    struct FrameUBOs
    {
        Vulk &vk;
        std::array<VulkUniformBuffer<XformsUBO>, MAX_FRAMES_IN_FLIGHT> xforms;
        std::array<VulkUniformBuffer<glm::vec3>, MAX_FRAMES_IN_FLIGHT> eyePos;
        FrameUBOs(Vulk &vk) : vk(vk)
        {
            for (auto &ubo : xforms)
            {
                ubo.init(vk);
            }
            for (auto &ubo : eyePos)
            {
                ubo.init(vk);
            }
        }
        ~FrameUBOs()
        {
            for (auto &ubo : xforms)
            {
                ubo.cleanup(vk);
            }
            for (auto &ubo : eyePos)
            {
                ubo.cleanup(vk);
            }
        }
    };
    FrameUBOs frameUBOs;

    struct MeshRenderBuffers
    {
        Vulk &vk;
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;

        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;

        MeshRenderBuffers(Vulk &vk, std::vector<Vertex> const &vertices, std::vector<uint32_t> const &indices) : vk(vk)
        {
            VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
            vk.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
            bufferSize = sizeof(indices[0]) * indices.size();
            vk.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
            copyToBuffer(vertices, indices);
        }

        void copyToBuffer(std::vector<Vertex> const &vertices, std::vector<uint32_t> const &indices)
        {
            VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
            vk.copyFromMemToBuffer(vertices.data(), vertexBuffer, bufferSize);

            bufferSize = sizeof(indices[0]) * indices.size();
            vk.copyFromMemToBuffer(indices.data(), indexBuffer, bufferSize);
        }

        ~MeshRenderBuffers()
        {
            vkDestroyBuffer(vk.device, vertexBuffer, nullptr);
            vkFreeMemory(vk.device, vertexBufferMemory, nullptr);
            vkDestroyBuffer(vk.device, indexBuffer, nullptr);
            vkFreeMemory(vk.device, indexBufferMemory, nullptr);
        }
    };

    VulkTextureView tileTextureView;
    VulkTextureView fenceOpacityView;
    VulkTextureView fenceColorView;
    VkSampler textureSampler;

    struct Mesh
    {
        Vulk &vk;
        VulkMesh mesh;
        std::unique_ptr<MeshRenderBuffers> meshRenderBuffers;
        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorPool descriptorPool;
        VkPipelineLayout pipelineLayout;
        VkPipeline graphicsPipeline;
        VkDescriptorSet descriptorSets[MAX_FRAMES_IN_FLIGHT];

        Mesh(Vulk &vk) : vk(vk)
        {
        }

        ~Mesh()
        {
            vkDestroyDescriptorSetLayout(vk.device, descriptorSetLayout, nullptr);
            vkDestroyDescriptorPool(vk.device, descriptorPool, nullptr);
            vkDestroyPipelineLayout(vk.device, pipelineLayout, nullptr);
            vkDestroyPipeline(vk.device, graphicsPipeline, nullptr);
        }
    };
    Mesh tile;
    Mesh fence;

public:
    BlendingWorld(Vulk &vk) : vk(vk),
                              frameUBOs(vk),
                              tileTextureView(vk, "Assets/Textures/ceramic_2-4K/ceramic_2_basecolor-4K.png"),
                              //fenceOpacityView(vk, "Assets/Textures/Fence003_4K-JPG/Fence003_4K_Opacity.jpg"),
                              fenceOpacityView(vk, "Assets/Textures/Fence007C_4K-JPG/Fence007C_4K_Opacity.jpg"),
                              fenceColorView(vk, "Assets/Textures/Fence007C_4K-JPG/Fence007C_4K_Color.jpg"),
                              tile(vk),
                              fence(vk)
    {
        textureSampler = vk.createTextureSampler();
        camera.lookAt(glm::vec3(0.0f, 0.f, 1.3f), glm::vec3(0.f, 0.f, 0.f));

        makeQuad(1.f, 1.f, 1, tile.mesh);
        makeQuad(1.f, 1.f, 1, fence.mesh);
        fence.mesh.xform(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.05f)));
        tile.mesh.name = "tile";
        fence.mesh.name = "fence";

        tile.meshRenderBuffers = std::make_unique<MeshRenderBuffers>(vk, tile.mesh.vertices, tile.mesh.indices);

        tile.descriptorSetLayout = VulkDescriptorSetLayoutBuilder(vk)
                                       .addUniformBuffer(VulkShaderBinding_XformsUBO, VK_SHADER_STAGE_VERTEX_BIT)
                                       .addUniformBuffer(VulkShaderBinding_EyePos, VK_SHADER_STAGE_FRAGMENT_BIT)
                                       .addSampler(VulkShaderBinding_TextureSampler)
                                       .build();

        tile.descriptorPool = VulkDescriptorPoolBuilder(vk)
                                  .addUniformBufferCount(MAX_FRAMES_IN_FLIGHT * 2)
                                  .addCombinedImageSamplerCount(MAX_FRAMES_IN_FLIGHT * 1)
                                  .build(MAX_FRAMES_IN_FLIGHT);

        VulkPipelineBuilder(vk)
            .addVertexShaderStage("Assets/Shaders/Vert/model.spv")
            .addFragmentShaderStage("Assets/Shaders/Frag/model.spv")
            .addVulkVertexInput(0)
            .setPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .setCullMode(VK_CULL_MODE_BACK_BIT)
            .setDepthTestEnabled(true)
            .setDepthWriteEnabled(true)
            .setBlendingEnabled(true)
            .build(tile.descriptorSetLayout, &tile.pipelineLayout, &tile.graphicsPipeline);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            tile.descriptorSets[i] = vk.createDescriptorSet(tile.descriptorSetLayout, tile.descriptorPool);
            VulkDescriptorSetUpdater(tile.descriptorSets[i])
                .addUniformBuffer(frameUBOs.xforms[i].buf, frameUBOs.xforms[i].getSize(), VulkShaderBinding_XformsUBO)
                .addUniformBuffer(frameUBOs.eyePos[i].buf, frameUBOs.eyePos[i].getSize(), VulkShaderBinding_EyePos)
                .addImageSampler(tileTextureView.textureImageView, textureSampler, VulkShaderBinding_TextureSampler)
                .update(vk.device);
        }

        fence.meshRenderBuffers = std::make_unique<MeshRenderBuffers>(vk, fence.mesh.vertices, fence.mesh.indices);

        fence.descriptorSetLayout = VulkDescriptorSetLayoutBuilder(vk)
                                       .addUniformBuffer(VulkShaderBinding_XformsUBO, VK_SHADER_STAGE_VERTEX_BIT)
                                       .addUniformBuffer(VulkShaderBinding_EyePos, VK_SHADER_STAGE_FRAGMENT_BIT)
                                       .addSampler(VulkShaderBinding_TextureSampler)
                                       .addSampler(VulkShaderBinding_TextureSampler2)
                                       .build();

        fence.descriptorPool = VulkDescriptorPoolBuilder(vk)
                                  .addUniformBufferCount(MAX_FRAMES_IN_FLIGHT * 2)
                                  .addCombinedImageSamplerCount(MAX_FRAMES_IN_FLIGHT * 2)
                                  .build(MAX_FRAMES_IN_FLIGHT);

        VulkPipelineBuilder(vk)
            .addVertexShaderStage("Assets/Shaders/Vert/blending.spv")
            .addFragmentShaderStage("Assets/Shaders/Frag/blending.spv")
            .addVulkVertexInput(0)
            .setPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .setCullMode(VK_CULL_MODE_BACK_BIT)
            .setDepthTestEnabled(true)
            .setDepthWriteEnabled(true)
            .setBlendingEnabled(true)
            .build(fence.descriptorSetLayout, &fence.pipelineLayout, &fence.graphicsPipeline);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            fence.descriptorSets[i] = vk.createDescriptorSet(fence.descriptorSetLayout, fence.descriptorPool);
            VulkDescriptorSetUpdater(fence.descriptorSets[i])
                .addUniformBuffer(frameUBOs.xforms[i].buf, frameUBOs.xforms[i].getSize(), VulkShaderBinding_XformsUBO)
                .addUniformBuffer(frameUBOs.eyePos[i].buf, frameUBOs.eyePos[i].getSize(), VulkShaderBinding_EyePos)
                .addImageSampler(fenceOpacityView.textureImageView, textureSampler, VulkShaderBinding_TextureSampler)
                .addImageSampler(fenceColorView.textureImageView, textureSampler, VulkShaderBinding_TextureSampler2)
                .update(vk.device);
        }
    }

    void updateXformsUBO(XformsUBO &ubo, VkViewport const &viewport)
    {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        time = 0.0f;
        ubo.world = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec3 fwd = camera.getForwardVec();
        glm::vec3 lookAt = camera.eye + fwd;
        glm::vec3 up = camera.getUpVec();
        ubo.view = glm::lookAt(camera.eye, lookAt, up);
        ubo.proj = glm::perspective(glm::radians(45.0f), viewport.width / (float)viewport.height, .01f, 10.0f);
        ubo.proj[1][1] *= -1;
    }

    void render(VkCommandBuffer commandBuffer, uint32_t currentFrame, VkViewport const &viewport, VkRect2D const &scissor)
    {
        VkDeviceSize offsets[] = {0};
        updateXformsUBO(*frameUBOs.xforms[currentFrame].mappedUBO, viewport);

        // tile
        *frameUBOs.eyePos[currentFrame].mappedUBO = camera.eye;
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tile.graphicsPipeline);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &tile.meshRenderBuffers->vertexBuffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, tile.meshRenderBuffers->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tile.pipelineLayout, 0, 1, &tile.descriptorSets[currentFrame], 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, (uint32_t)tile.mesh.indices.size(), 1, 0, 0, 0);

        // fence
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, fence.graphicsPipeline);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &fence.meshRenderBuffers->vertexBuffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, fence.meshRenderBuffers->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, fence.pipelineLayout, 0, 1, &fence.descriptorSets[currentFrame], 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, (uint32_t)fence.mesh.indices.size(), 1, 0, 0, 0);
    }

    ~BlendingWorld()
    {
        vkDestroySampler(vk.device, textureSampler, nullptr);
    }
};

class Blending : public Vulk
{
protected:
    std::unique_ptr<BlendingWorld> world;
    void init() override
    {
        world = std::make_unique<BlendingWorld>(*this);
    }

    void drawFrame(VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer) override
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VK_CALL(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = frameBuffer;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.1f, 0.0f, 0.1f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        {
            world->render(commandBuffer, currentFrame, viewport, scissor);
        }
        vkCmdEndRenderPass(commandBuffer);

        VK_CALL(vkEndCommandBuffer(commandBuffer));
    }

    virtual void cleanup()
    {
        world.reset();
    }

    void keyCallback(int key, int scancode, int action, int mods)
    {
        VulkCamera &camera = world->camera;
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
        {
            glm::vec3 fwd = camera.getForwardVec();
            glm::vec3 right = camera.getRightVec();
            glm::vec3 up = camera.getUpVec();
            float move = .005f;
            bool handled = true;
            if (key == GLFW_KEY_W)
            {
                camera.eye += move * fwd;
            }
            else if (key == GLFW_KEY_A)
            {
                camera.eye -= move * right;
            }
            else if (key == GLFW_KEY_S)
            {
                camera.eye -= move * fwd;
            }
            else if (key == GLFW_KEY_D)
            {
                camera.eye += move * right;
            }
            else if (key == GLFW_KEY_Q)
            {
                camera.eye -= move * up;
            }
            else if (key == GLFW_KEY_E)
            {
                camera.eye += move * up;
            }
            else if (key == GLFW_KEY_LEFT)
            {
                camera.yaw -= 15.0f;
            }
            else if (key == GLFW_KEY_RIGHT)
            {
                camera.yaw += 15.0f;
            }
            else if (key == GLFW_KEY_UP)
            {
                camera.pitch += 15.0f;
            }
            else if (key == GLFW_KEY_DOWN)
            {
                camera.pitch -= 15.0f;
            }
            else
            {
                handled = false;
            }
            if (handled)
            {
                camera.yaw = fmodf(camera.yaw, 360.0f);
                camera.pitch = fmodf(camera.pitch, 360.0f);
                std::cout << "eye: " << camera.eye.x << ", " << camera.eye.y << ", " << camera.eye.z << " yaw: " << camera.yaw << " pitch: " << camera.pitch << std::endl;
                return;
            }
        }
        Vulk::keyCallback(key, scancode, action, mods);
    }
};
