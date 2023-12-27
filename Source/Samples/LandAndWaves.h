#pragma once

#include "Vulk/Vulk.h"
#include "Vulk/VulkGeo.h"
#include "Vulk/VulkActor.h"
#include "Vulk/VulkCamera.h"
#include "Vulk/VulkPipelineBuilder.h"
#include "Vulk/VulkDescriptorPoolBuilder.h"

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

    VkDescriptorPool actorsDescriptorPool;

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
            vk.copyFromMemToBuffer(vertices.data(), vertexBuffer, bufferSize);

            bufferSize = sizeof(indices[0]) * indices.size();
            vk.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
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

    VulkMesh waves;
    MeshRender wavesRender;
    VkDescriptorSetLayout wavesDescriptorSetLayout;
    VkDescriptorSet wavesDescriptorSet;
    VkBuffer wavesVertexBuffer;
    VkBuffer wavesIndexBuffer;


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

    bool rotateWorld = false;

    float getTerrainHeight(Vertex const &v) {
        return 0.3f * (v.pos.z * sinf(0.1f * v.pos.x) + v.pos.x * cosf(0.1f * v.pos.z));
    }

public:
    void init() override {
        actorsDescriptorSetLayout = VulkDescriptorSetLayoutBuilder()
            .addUniformBuffer(0)
            .addSampler(1)
            .addStorageBuffer(2, VK_SHADER_STAGE_VERTEX_BIT)
            .build(*this);

        VulkPipelineBuilder pipelineBuilder(*this);
        pipelineBuilder.addVertexShaderStage("Assets/Shaders/Vert/terrain.spv");
        pipelineBuilder.addVertexInputBindingDescription(0, sizeof(Vertex));
        pipelineBuilder.addVertexInputFieldVec3(0, Vertex::PosBinding, offsetof(Vertex, pos));
        pipelineBuilder.addVertexInputFieldVec3(0, Vertex::NormalBinding, offsetof(Vertex, normal));
        pipelineBuilder.addVertexInputFieldVec3(0, Vertex::TangentBinding, offsetof(Vertex, tangent));
        pipelineBuilder.addVertexInputFieldVec2(0, Vertex::TexCoordBinding, offsetof(Vertex, texCoord));
        pipelineBuilder.addFragmentShaderStage("Assets/Shaders/Frag/terrain.spv");
        pipelineBuilder.build(renderPass, actorsDescriptorSetLayout, actorsPipelineLayout, actorsGraphicsPipeline);


        camera.lookAt(glm::vec3(15.f, 120.f, 170.f), glm::vec3(0.f, 0.f, 0.f));

        VulkMesh terrain;
        makeGrid(160, 160, 50, 50, terrain);
        for (auto &v : terrain.vertices) {
            v.pos.y = getTerrainHeight(v);
        }

        VulkMeshRef terrainRef = meshAccumulator.appendMesh(terrain);
        meshActors["terrain"] = {
            meshAccumulator.appendMesh(terrain),
            {{"terrain0", glm::translate(glm::mat4(1.0f), glm::vec3(0.f, 0.f, 0.f))}}
        };

        makeGrid(160, 160, 50, 50, waves);

        actorsRender.init(*this, meshAccumulator.vertices, meshAccumulator.indices);
        wavesRender.init(*this, waves.vertices, waves.indices);

        createTextureImage("Assets/Textures/uv_checker.png", textureImageMemory, textureImage);
        textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
        textureSampler = createTextureSampler();
        for (auto &ubo: ubos) {
            ubo.createUniformBuffers(*this);
        }
        uint32_t numMeshes = static_cast<uint32_t>(meshActors.size());
        actorsDescriptorPool = VulkDescriptorPoolBuilder()
            .addUniformBufferCount(MAX_FRAMES_IN_FLIGHT * numMeshes)
            .addCombinedImageSamplerCount(MAX_FRAMES_IN_FLIGHT * numMeshes)
            .addStorageBufferCount(MAX_FRAMES_IN_FLIGHT * numMeshes)
            .build(device);


        for (auto &meshActor : meshActors) {
            auto &meshRenderInfo = meshActor.second;
            uint32_t numActors = static_cast<uint32_t>(meshRenderInfo.actors.size());
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                MeshFrameResources &res = meshRenderInfo.meshRenderData[i];
                // map the actor xforms into a mem-mapped SSBO
                res.buf.createAndMap(*this, numActors);

                // create the descriptor set
                createDescriptorSet(actorsDescriptorSetLayout, actorsDescriptorPool, res.descriptorSet);
                VulkDescriptorSetUpdater descriptorSetUpdater(res.descriptorSet);
                descriptorSetUpdater.addUniformBuffer(ubos[i].buf, sizeof(UniformBufferObject), 0);
                descriptorSetUpdater.addImageSampler(textureImageView, textureSampler, 1);
                descriptorSetUpdater.addStorageBuffer(res.buf.buf, sizeof(ActorSSBOElt) * numActors, 2);
                descriptorSetUpdater.update(device);
            }
        }

        // waves
        wavesDescriptorSetLayout = VulkDescriptorSetLayoutBuilder()
            .addUniformBuffer(0)
            .build(*this);
        wavesDescriptorPool = VulkDescriptorPoolBuilder()
            .addUniformBufferCount()
        createDescriptorSet(wavesDescriptorSetLayout, actorsDescriptorPool, wavesDescriptorSet);
        VulkDescriptorSetUpdater wavesDescriptorSetUpdater(wavesDescriptorSet);
        wavesDescriptorSetUpdater.update(device);
        createDescriptorSet(wavesDescriptorSetLayout, actorsDescriptorPool, wavesDescriptorSet);

    }

private:

    void updateUniformBuffer(UniformBufferObject &ubo) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        if (!rotateWorld) {
            time = 0.0f;
        }
        ubo.world = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::vec3 fwd = camera.getForwardVec();
        glm::vec3 lookAt = camera.eye + fwd;
        glm::vec3 up = camera.getUpVec();
        ubo.view = glm::lookAt(camera.eye, lookAt, up);
        ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 1.f, 4000.0f);
        ubo.proj[1][1] *= -1;
    }

    void drawFrame(VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer) override {
        updateUniformBuffer(*ubos[currentFrame].mappedUBO);
        for (auto &meshActor : meshActors) {
           meshActor.second.updateActorSSBO(currentFrame);
        }

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

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actorsGraphicsPipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkBuffer vertexBuffers[] = { actorsRender.vertexBuffer, wavesRender.vertexBuffer };
        VkDeviceSize offsets[] = { 0, 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(commandBuffer, actorsRender.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        for (auto &meshActor: meshActors) {
            MeshRenderInfo &meshRenderInfo = meshActor.second;
            MeshFrameResources &res = meshRenderInfo.meshRenderData[currentFrame];
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actorsPipelineLayout, 0, 1, &res.descriptorSet, 0, nullptr);
            vkCmdDrawIndexed(commandBuffer, meshRenderInfo.meshRef.indexCount, (uint32_t)meshRenderInfo.actors.size(), meshRenderInfo.meshRef.firstIndex, meshRenderInfo.meshRef.firstVertex, 0);
        }

        // vkCmdBindIndexBuffer(commandBuffer, wavesRender.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        // vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actorsPipelineLayout, 0, 1, &wavesDescriptorSet, 0, nullptr);
        // vkCmdDrawIndexed(commandBuffer, (uint32_t)waves.indices.size(), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        VK_CALL(vkEndCommandBuffer(commandBuffer));
    }

    void cleanup() override {
        vkDestroyPipeline(device, actorsGraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, actorsPipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

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
        wavesRender.cleanup(*this);
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
