#pragma once

#include "Vulk/Vulk.h"
#include "Vulk/VulkGeo.h"
#include "Vulk/VulkActor.h"
#include "Vulk/VulkCamera.h"
#include "Vulk/VulkPipelineBuilder.h"
#include "Vulk/VulkDescriptorPoolBuilder.h"
#include "Vulk/VulkUniformBuffer.h"
#include "Vulk/VulkStorageBuffer.h"

class Lighting : public Vulk {
    VulkCamera camera;

    struct XformsUBO {
        alignas(16) glm::mat4 world;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    std::array<VulkUniformBuffer<XformsUBO>, MAX_FRAMES_IN_FLIGHT> xformsUBOs;
    std::array<VulkUniformBuffer<glm::vec3>, MAX_FRAMES_IN_FLIGHT> eyePosUBOs;

    struct ActorSSBOElt {
        glm::mat4 xform;
    };

    struct MeshRenderInfo {
        VulkMeshRef meshRef; // what we're drawing
        std::vector<VulkActor> actors; // the actors that use this mesh
        std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets; // the per-frame descriptor sets
        std::array<VulkStorageBuffer<ActorSSBOElt>, MAX_FRAMES_IN_FLIGHT> ssbos; // the per-frame actor xforms
        void updateActorSSBO(uint32_t curFrame) {
            auto &res = ssbos[curFrame];
            for (int i = 0; i < actors.size(); i++) {
                res.mappedObjs[i].xform = actors[i].xform;
            }
        }
        void cleanup(VkDevice dev) {
            for (auto &res : ssbos) {
                res.cleanup(dev);
            }
        }
    };
    
    VkDescriptorPool actorsDescriptorPool;
    std::unordered_map<char const *, MeshRenderInfo> meshActors;

    struct MeshRender {
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;

        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;

        void init(Vulk &vk, std::vector<Vertex> const &vertices, std::vector<uint32_t> const &indices) {
            VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();    
            vk.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
            bufferSize = sizeof(indices[0]) * indices.size();
            vk.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
            copyToBuffer(vk, vertices, indices);
        }

        void copyToBuffer(Vulk &vk, std::vector<Vertex> const &vertices, std::vector<uint32_t> const &indices) {
            VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();    
            vk.copyFromMemToBuffer(vertices.data(), vertexBuffer, bufferSize);

            bufferSize = sizeof(indices[0]) * indices.size();
            vk.copyFromMemToBuffer(indices.data(), indexBuffer, bufferSize);
        }

        void cleanup(Vulk &vk) {
            vkDestroyBuffer(vk.device, vertexBuffer, nullptr);
            vkFreeMemory(vk.device, vertexBufferMemory, nullptr);
            vkDestroyBuffer(vk.device, indexBuffer, nullptr);
            vkFreeMemory(vk.device, indexBufferMemory, nullptr);
        }
    };

    VkDescriptorSetLayout actorsDescriptorSetLayout;
    MeshRender actorsRender;
    VkPipelineLayout actorsPipelineLayout;
    VkPipeline actorsGraphicsPipeline;

    struct MeshAccumulator {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        VulkMeshRef appendMesh(VulkMesh const &mesh) {
            uint32_t vertexOffset = static_cast<uint32_t>(vertices.size());
            vertices.insert(vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
            uint32_t indexOffset = static_cast<uint32_t>(indices.size());
            indices.insert(indices.end(), mesh.indices.begin(), mesh.indices.end());
            return VulkMeshRef{mesh.name, vertexOffset, indexOffset, static_cast<uint32_t>(mesh.indices.size())};
        }
    } meshAccumulator;
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    bool rotateWorld = true;

    float getTerrainHeight(Vertex const &v) {
        return 0.3f * (v.pos.z * sinf(0.1f * v.pos.x) + v.pos.x * cosf(0.1f * v.pos.z));
    }

    struct Material {
        glm::vec4 diffuse;
        glm::vec3 fresnelR0;
        float roughness;
    };
    Material material = {
        {1.0f, 1.0f, 1.0f, 1.f},
        {0.01f, 0.01f, 0.01f},
        0.5f
    };
    VulkStorageBuffer<Material> materialsSSBO;

    struct Light {
        glm::vec3 pos;          // point light only
        glm::vec4 color;        // color of light
        float falloffStart;     // point/spot light only
        float falloffEnd;       // point/spot light only: negative means no falloff 
        glm::vec3 direction;    // directional/spot light only
        float spotPower;        // spotlight only
    };
    Light light = {
        {0.0f, 100.0f, 0.0f},
        {1.0f, 1.0f, 1.0f, 1.0f},
        0.0f,
        0.0f,
        {0.0f, 0.0f, 0.0f},
        0.0f,
    };
    VulkStorageBuffer<Light> lightsSSBO;

public:
    void init() override {
        camera.lookAt(glm::vec3(0.f, 1.2f, 2.5f), glm::vec3(0.f, 0.f, 0.f));

        createTextureImage("Assets/Textures/uv_checker.png", textureImageMemory, textureImage);
        textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
        textureSampler = createTextureSampler();
        for (auto &ubo: xformsUBOs) {
            ubo.createUniformBuffers(*this);
        }
        for (auto &ubo: eyePosUBOs) {
            ubo.createUniformBuffers(*this);
        }

        // create the materials SSBO
        materialsSSBO.createAndMap(*this, 1);
        materialsSSBO.mappedObjs[0] = material;

        // create the lights SSBO
        lightsSSBO.createAndMap(*this, 1);
        lightsSSBO.mappedObjs[0] = light;

        actorsDescriptorSetLayout = VulkDescriptorSetLayoutBuilder()
            .addUniformBuffer(VulkShaderBinding_XformsUBO, VK_SHADER_STAGE_VERTEX_BIT)
            .addUniformBuffer(VulkShaderBinding_EyePos, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addSampler(VulkShaderBinding_Sampler)
            .addStorageBuffer(VulkShaderBinding_Actors, VK_SHADER_STAGE_VERTEX_BIT)
            .addStorageBuffer(VulkShaderBinding_Lights, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addStorageBuffer(VulkShaderBinding_Materials, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build(*this);

        VulkPipelineBuilder(*this)
            .addVertexShaderStage("Assets/Shaders/Vert/light.spv")
            .addVertexInputBindingDescription(0, sizeof(Vertex))
            .addVertexInputFieldVec3(0, Vertex::PosBinding, offsetof(Vertex, pos))
            .addVertexInputFieldVec3(0, Vertex::NormalBinding, offsetof(Vertex, normal))
            .addVertexInputFieldVec3(0, Vertex::TangentBinding, offsetof(Vertex, tangent))
            .addVertexInputFieldVec2(0, Vertex::TexCoordBinding, offsetof(Vertex, texCoord))
            .addFragmentShaderStage("Assets/Shaders/Frag/light.spv")
            .build(actorsDescriptorSetLayout, actorsPipelineLayout, actorsGraphicsPipeline);

        VulkMesh sphere;
        makeGeoSphere(1.f, 3, sphere);
        VulkMeshRef sphereRef = meshAccumulator.appendMesh(sphere);
        meshActors["sphere"] = {
            meshAccumulator.appendMesh(sphere),
            {{"sphere0", glm::translate(glm::mat4(1.0f), glm::vec3(0.f, 0.f, 0.f))}}
        };

        actorsRender.init(*this, meshAccumulator.vertices, meshAccumulator.indices);
        uint32_t numMeshes = static_cast<uint32_t>(meshActors.size());
        actorsDescriptorPool = VulkDescriptorPoolBuilder()
            .addUniformBufferCount(MAX_FRAMES_IN_FLIGHT * numMeshes * 2)
            .addCombinedImageSamplerCount(MAX_FRAMES_IN_FLIGHT * numMeshes)
            .addStorageBufferCount(MAX_FRAMES_IN_FLIGHT * numMeshes * 3)
            .build(device, MAX_FRAMES_IN_FLIGHT * numMeshes);

        for (auto &meshActor : meshActors) {
            auto &meshRenderInfo = meshActor.second;
            uint32_t numActors = static_cast<uint32_t>(meshRenderInfo.actors.size());
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                // map the actor xforms into a mem-mapped SSBO
                meshRenderInfo.ssbos[i].createAndMap(*this, numActors);

                // create the descriptor set
                meshRenderInfo.descriptorSets[i] = createDescriptorSet(actorsDescriptorSetLayout, actorsDescriptorPool);
                VulkDescriptorSetUpdater(meshRenderInfo.descriptorSets[i])
                    .addUniformBuffer(xformsUBOs[i].buf, xformsUBOs[i].getSize(), VulkShaderBinding_XformsUBO)
                    .addUniformBuffer(eyePosUBOs[i].buf, eyePosUBOs[i].getSize(), VulkShaderBinding_EyePos)
                    .addImageSampler(textureImageView, textureSampler, VulkShaderBinding_Sampler)
                    .addStorageBuffer(meshRenderInfo.ssbos[i].buf, sizeof(ActorSSBOElt) * numActors, VulkShaderBinding_Actors)
                    .addStorageBuffer(lightsSSBO.buf, sizeof(Light), VulkShaderBinding_Lights)
                    .addStorageBuffer(materialsSSBO.buf, sizeof(Material), VulkShaderBinding_Materials)
                    .update(device);
            }
        }
    }

private:
    VkDescriptorSet createDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool) {
        VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = layouts;

        VkDescriptorSet descriptorSet;
        VK_CALL(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
        return descriptorSet;
    }


    void updateXformsUBO(XformsUBO &ubo) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        if (!rotateWorld) {
            time = 0.0f;
        }
        ubo.world = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec3 fwd = camera.getForwardVec();
        glm::vec3 lookAt = camera.eye + fwd;
        glm::vec3 up = camera.getUpVec();
        ubo.view = glm::lookAt(camera.eye, lookAt, up);
        ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 1.f, 4000.0f);
        ubo.proj[1][1] *= -1;
    }

    void drawFrame(VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer) override {
        updateXformsUBO(*xformsUBOs[currentFrame].mappedUBO);
        *eyePosUBOs[currentFrame].mappedUBO = camera.eye;

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VK_CALL(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = frameBuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapChainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
        clearValues[1].depthStencil = { 1.0f, 0 };

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
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;

        VkDeviceSize offsets[] = {0};

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // actors
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actorsGraphicsPipeline);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &actorsRender.vertexBuffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, actorsRender.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        for (auto &meshActor: meshActors) {
            MeshRenderInfo &meshRenderInfo = meshActor.second;
            meshRenderInfo.updateActorSSBO(currentFrame); // is this safe? need to understand synchroniation better...;
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actorsPipelineLayout, 0, 1, &meshRenderInfo.descriptorSets[currentFrame], 0, nullptr);
            vkCmdDrawIndexed(commandBuffer, meshRenderInfo.meshRef.indexCount, (uint32_t)meshRenderInfo.actors.size(), meshRenderInfo.meshRef.firstIndex, meshRenderInfo.meshRef.firstVertex, 0);
        }

        vkCmdEndRenderPass(commandBuffer);

        VK_CALL(vkEndCommandBuffer(commandBuffer));
    }

    void cleanup() override {
        vkDestroyPipeline(device, actorsGraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, actorsPipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

        for (auto ubo: xformsUBOs) {
            ubo.cleanup(*this);
        }

        for (auto ubo: eyePosUBOs) {
            ubo.cleanup(*this);
        }

        for (auto &meshActor : meshActors) {
            meshActor.second.cleanup(device);
        }

        vkDestroyDescriptorSetLayout(device, actorsDescriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(device, actorsDescriptorPool, nullptr);
        vkDestroySampler(device, textureSampler, nullptr);
        vkDestroyImageView(device, textureImageView, nullptr);
        vkDestroyImage(device, textureImage, nullptr);
        vkFreeMemory(device, textureImageMemory, nullptr);
        actorsRender.cleanup(*this);
        materialsSSBO.cleanup(device);
        lightsSSBO.cleanup(device);
    }

    void keyCallback(int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            glm::vec3 fwd = camera.getForwardVec();
            glm::vec3 right = camera.getRightVec();
            glm::vec3 up = camera.getUpVec();
            float move = 1.f;
            bool handled = true;
            if (key == GLFW_KEY_W) {
                camera.eye += move * fwd;
            } else if (key == GLFW_KEY_A) {
                camera.eye -= move * right;
            } else if (key == GLFW_KEY_S) {
                camera.eye -= move * fwd;
            } else if (key == GLFW_KEY_D) {
                camera.eye += move * right;
            } else if (key == GLFW_KEY_Q) {
                camera.eye -= move * up;
            } else if (key == GLFW_KEY_E) {
                camera.eye += move * up;
            } else if (key == GLFW_KEY_LEFT) {
                camera.yaw -= 15.0f;
            } else if (key == GLFW_KEY_RIGHT) {
                camera.yaw += 15.0f;
            } else if (key == GLFW_KEY_UP) {
                camera.pitch += 15.0f;
            } else if (key == GLFW_KEY_DOWN) {
                camera.pitch -= 15.0f;
            } else {
                handled = false;
            }
            if (handled) {
                camera.yaw = fmodf(camera.yaw, 360.0f);
                camera.pitch = fmodf(camera.pitch, 360.0f);
                std::cout << "eye: " << camera.eye.x << ", " << camera.eye.y << ", " << camera.eye.z << " yaw: " << camera.yaw << " pitch: " << camera.pitch << std::endl;
                return;
            }
        }
        Vulk::keyCallback(key, scancode, action, mods);
    }
};
