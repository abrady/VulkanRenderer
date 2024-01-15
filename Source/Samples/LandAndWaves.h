#pragma once

#include "Vulk/Vulk.h"
#include "Vulk/VulkGeo.h"
#include "Vulk/VulkActor.h"
#include "Vulk/VulkCamera.h"
#include "Vulk/VulkPipelineBuilder.h"
#include "Vulk/VulkDescriptorPoolBuilder.h"
#include "Vulk/VulkDescriptorSetUpdater.h"

class LandAndWaves : public Vulk {
    struct UniformBufferObject {
        alignas(16) glm::mat4 world;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };
    VulkCamera camera;

    struct ActorSSBOElt {
        glm::mat4 xform;
    };

    struct ActorSSBO {
        VkBuffer buf;
        VkDeviceMemory mem;
        ActorSSBOElt* mappedActorElts; // contiguous array of memory mapped actor instance info

        void createAndMap(Vulk &vk, uint32_t numActors) {
            VkDeviceSize bufferSize = sizeof(ActorSSBOElt) * numActors;
            vk.createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buf, mem);
            vkMapMemory(vk.device, mem, 0, bufferSize, 0, (void**)&mappedActorElts);
        }
        void cleanup(VkDevice dev) {
            vkDestroyBuffer(dev, buf, nullptr);
            vkFreeMemory(dev, mem, nullptr);
        }
    };

    struct MeshFrameResources {
        VkDescriptorSet descriptorSet;
        ActorSSBO buf;

        void cleanup(VkDevice dev) {
            buf.cleanup(dev);
        }
    };
    struct MeshRenderInfo {
        VulkMeshRef meshRef; // what we're drawing
        std::vector<VulkActor> actors; // the actors that use this mesh
        std::array<MeshFrameResources, MAX_FRAMES_IN_FLIGHT> meshRenderData; // the per-frame data
        void updateActorSSBO(uint32_t curFrame) {
            MeshFrameResources &res = meshRenderData[curFrame];
            for (int i = 0; i < actors.size(); i++) {
                ActorSSBOElt ubo{};
                ubo.xform = actors[i].xform;
                res.buf.mappedActorElts[i] = ubo;
            }
        }
        void cleanup(VkDevice dev) {
            for (auto &res : meshRenderData) {
                res.cleanup(dev);
            }
        }
    };
    VkDescriptorPool actorsDescriptorPool;
    std::unordered_map<char const *, MeshRenderInfo> meshActors;

    struct UBO {
        VkBuffer buf;
        VkDeviceMemory mem;
        UniformBufferObject* mappedUBO;
        void createUniformBuffers(Vulk &vk) {
            VkDeviceSize bufferSize = sizeof(UniformBufferObject);
            vk.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buf, mem);
            vkMapMemory(vk.device, mem, 0, bufferSize, 0, (void**)&mappedUBO);
        }
    };
    std::array<UBO, MAX_FRAMES_IN_FLIGHT> ubos;

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

    VkPipelineLayout wavesPipelineLayout;
    VkPipeline wavesGraphicsPipeline;
    VkDescriptorPool wavesDescriptorPool;
    VkDescriptorSetLayout wavesDescriptorSetLayout;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> wavesDescriptorSets;
    VulkMesh wavesMesh;
    std::array<MeshRender, MAX_FRAMES_IN_FLIGHT> wavesRender;

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

public:
    void init() override {
        camera.lookAt(glm::vec3(15.f, 120.f, 170.f), glm::vec3(0.f, 0.f, 0.f));

        VulkMesh terrain;
        makeGrid(160, 160, 50, 50, terrain);
        for (auto &v : terrain.vertices) {
            v.pos.y = getTerrainHeight(v);
        }

        createTextureImage("Assets/Textures/uv_checker.png", textureImageMemory, textureImage);
        textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
        textureSampler = createTextureSampler();
        for (auto &ubo: ubos) {
            ubo.createUniformBuffers(*this);
        }

        actorsDescriptorSetLayout = VulkDescriptorSetLayoutBuilder(*this)
            .addUniformBuffer(0, VK_SHADER_STAGE_VERTEX_BIT)
            .addSampler(1)
            .addStorageBuffer(2, VK_SHADER_STAGE_VERTEX_BIT)
            .build();

        VulkPipelineBuilder(*this)
            .addVertexShaderStageDeprecated("Source/Shaders/Vert/terrain.spv")
            .addVertexInputBindingDescription(0, sizeof(Vertex))
            .addVertexInputFieldVec3(0, Vertex::PosBinding, offsetof(Vertex, pos))
            .addVertexInputFieldVec3(0, Vertex::NormalBinding, offsetof(Vertex, normal))
            .addVertexInputFieldVec3(0, Vertex::TangentBinding, offsetof(Vertex, tangent))
            .addVertexInputFieldVec2(0, Vertex::TexCoordBinding, offsetof(Vertex, texCoord))
            .addFragmentShaderStageDeprecated("Source/Shaders/Frag/terrain.spv")
            .build(actorsDescriptorSetLayout, &actorsPipelineLayout, &actorsGraphicsPipeline);

        VulkMeshRef terrainRef = meshAccumulator.appendMesh(terrain);
        meshActors["terrain"] = {
            meshAccumulator.appendMesh(terrain),
            {{"terrain0", glm::translate(glm::mat4(1.0f), glm::vec3(0.f, 0.f, 0.f))}}
        };

        actorsRender.init(*this, meshAccumulator.vertices, meshAccumulator.indices);
        uint32_t numMeshes = static_cast<uint32_t>(meshActors.size());
        actorsDescriptorPool = VulkDescriptorPoolBuilder(*this)
            .addUniformBufferCount(MAX_FRAMES_IN_FLIGHT * numMeshes)
            .addCombinedImageSamplerCount(MAX_FRAMES_IN_FLIGHT * numMeshes)
            .addStorageBufferCount(MAX_FRAMES_IN_FLIGHT * numMeshes)
            .build(MAX_FRAMES_IN_FLIGHT * numMeshes);

        for (auto &meshActor : meshActors) {
            auto &meshRenderInfo = meshActor.second;
            uint32_t numActors = static_cast<uint32_t>(meshRenderInfo.actors.size());
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                MeshFrameResources &res = meshRenderInfo.meshRenderData[i];
                // map the actor xforms into a mem-mapped SSBO
                res.buf.createAndMap(*this, numActors);

                // create the descriptor set
                res.descriptorSet = createDescriptorSet(actorsDescriptorSetLayout, actorsDescriptorPool);
                VulkDescriptorSetUpdater(res.descriptorSet)
                    .addUniformBuffer(ubos[i].buf, sizeof(UniformBufferObject), 0)
                    .addImageSampler(textureImageView, textureSampler, 1)
                    .addStorageBuffer(res.buf.buf, sizeof(ActorSSBOElt) * numActors, 2)
                    .update(device);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // waves
        
        makeGrid(160, 160, 50, 50, wavesMesh);
        for (auto &render: wavesRender) {
            render.init(*this, wavesMesh.vertices, wavesMesh.indices);
        }

        wavesDescriptorSetLayout = VulkDescriptorSetLayoutBuilder(*this)
            .addUniformBuffer(0, VK_SHADER_STAGE_VERTEX_BIT)
            .addSampler(1)
            .build();

        VulkPipelineBuilder(*this)
            .addVertexShaderStageDeprecated("Source/Shaders/Vert/waves.spv")
            .addVertexInputBindingDescription(0,sizeof(Vertex))
            .addVertexInputFieldVec3(0, Vertex::PosBinding, offsetof(Vertex, pos))
            .addVertexInputFieldVec3(0, Vertex::NormalBinding, offsetof(Vertex, normal))
            .addVertexInputFieldVec3(0, Vertex::TangentBinding, offsetof(Vertex, tangent))
            .addVertexInputFieldVec2(0, Vertex::TexCoordBinding, offsetof(Vertex, texCoord))
            .addFragmentShaderStageDeprecated("Source/Shaders/Frag/waves.spv")
            .build(wavesDescriptorSetLayout, &wavesPipelineLayout, &wavesGraphicsPipeline);

        wavesDescriptorPool = VulkDescriptorPoolBuilder(*this)
            .addUniformBufferCount(MAX_FRAMES_IN_FLIGHT)
            .addCombinedImageSamplerCount(MAX_FRAMES_IN_FLIGHT)
            .build(MAX_FRAMES_IN_FLIGHT);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            wavesDescriptorSets[i] = createDescriptorSet(wavesDescriptorSetLayout, wavesDescriptorPool);
            VulkDescriptorSetUpdater(wavesDescriptorSets[i])
                .addUniformBuffer(ubos[i].buf, sizeof(UniformBufferObject), 0)
                .addImageSampler(textureImageView, textureSampler, 1)
                .update(device);
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


    void updateUniformBuffer(UniformBufferObject &ubo) {
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

    void wavesTick() {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        for (auto &v : wavesMesh.vertices) {
            v.pos.y = 0.05f * (v.pos.z * sinf(0.1f * v.pos.x + time) + v.pos.x * cosf(0.1f * v.pos.z + time));
        }

        // update the buffer
        wavesRender[(currentFrame + 1) % MAX_FRAMES_IN_FLIGHT].copyToBuffer(*this, wavesMesh.vertices, wavesMesh.indices);
    }

    void drawFrame(VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer) override {
        wavesTick();
        updateUniformBuffer(*ubos[currentFrame].mappedUBO);

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

        // waves
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wavesGraphicsPipeline);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &wavesRender[currentFrame].vertexBuffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, wavesRender[currentFrame].indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wavesPipelineLayout, 0, 1, &wavesDescriptorSets[currentFrame], 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, (uint32_t)wavesMesh.indices.size(), 1, 0, 0, 0);

        // actors
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actorsGraphicsPipeline);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &actorsRender.vertexBuffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, actorsRender.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        for (auto &meshActor: meshActors) {
            MeshRenderInfo &meshRenderInfo = meshActor.second;
            meshRenderInfo.updateActorSSBO(currentFrame); // is this safe? need to understand synchroniation better...
            MeshFrameResources &res = meshRenderInfo.meshRenderData[currentFrame];
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actorsPipelineLayout, 0, 1, &res.descriptorSet, 0, nullptr);
            vkCmdDrawIndexed(commandBuffer, meshRenderInfo.meshRef.indexCount, (uint32_t)meshRenderInfo.actors.size(), meshRenderInfo.meshRef.firstIndex, meshRenderInfo.meshRef.firstVertex, 0);
        }

        vkCmdEndRenderPass(commandBuffer);

        VK_CALL(vkEndCommandBuffer(commandBuffer));
    }

    void cleanup() override {
        vkDestroyPipeline(device, actorsGraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, actorsPipelineLayout, nullptr);

        for (auto ubo: ubos) {
            vkDestroyBuffer(device, ubo.buf, nullptr);
            vkFreeMemory(device, ubo.mem, nullptr);
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

        vkDestroyDescriptorSetLayout(device, wavesDescriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(device, wavesDescriptorPool, nullptr);
        vkDestroyPipeline(device, wavesGraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, wavesPipelineLayout, nullptr);
        for (auto &render : wavesRender) {
            render.cleanup(*this);
        }
    }

    void handleEvents() override {
        // override this to call things like glfwGetKey and glfwGetMouseButton
    }

    void keyCallback(int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            glm::vec3 fwd = camera.getForwardVec();
            glm::vec3 right = camera.getRightVec();
            glm::vec3 up = camera.getUpVec();
            float move = 10.f;
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
