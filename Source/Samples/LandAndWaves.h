#pragma once

#include "Vulk/Vulk.h"
#include "Vulk/VulkGeo.h"
#include "Vulk/VulkActor.h"
#include "Vulk/VulkCamera.h"

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

    VkDescriptorPool descriptorPool;

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

    VulkMesh waves;
    MeshRender wavesRender;
    VkDescriptorSetLayout wavesDescriptorSetLayout;
    VkDescriptorSet wavesDescriptorSet;
    VkBuffer wavesVertexBuffer;
    VkBuffer wavesIndexBuffer;

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

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
        VulkDescriptorSetLayoutBuilder actorsDescriptorSetLayoutBuilder;
        actorsDescriptorSetLayoutBuilder.addUniformBuffer(0);
        actorsDescriptorSetLayoutBuilder.addSampler(1);
        actorsDescriptorSetLayoutBuilder.addStorageBuffer(2, VK_SHADER_STAGE_VERTEX_BIT);
        actorsDescriptorSetLayoutBuilder.build(*this, actorsDescriptorSetLayout);

        createGraphicsPipeline();

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
        createDescriptorPool(static_cast<uint32_t>(meshActors.size()) + 1); // 1 for the waves

        for (auto &meshActor : meshActors) {
            auto &meshRenderInfo = meshActor.second;
            uint32_t numActors = static_cast<uint32_t>(meshRenderInfo.actors.size());
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                MeshFrameResources &res = meshRenderInfo.meshRenderData[i];
                // map the actor xforms into a mem-mapped SSBO
                res.buf.createAndMap(*this, numActors);

                // create the descriptor set
                createDescriptorSet(actorsDescriptorSetLayout, descriptorPool, res.descriptorSet);
                VulkDescriptorSetUpdater descriptorSetUpdater(res.descriptorSet);
                descriptorSetUpdater.addUniformBuffer(ubos[i].buf, sizeof(UniformBufferObject), 0);
                descriptorSetUpdater.addImageSampler(textureImageView, textureSampler, 1);
                descriptorSetUpdater.addStorageBuffer(res.buf.buf, sizeof(ActorSSBOElt) * numActors, 2);
                descriptorSetUpdater.update(device);
            }
        }

        // waves
        VulkDescriptorSetLayoutBuilder wavesDescriptorSetLayoutBuilder;
        wavesDescriptorSetLayoutBuilder.build(*this, wavesDescriptorSetLayout);
        createDescriptorSet(wavesDescriptorSetLayout, descriptorPool, wavesDescriptorSet);
        VulkDescriptorSetUpdater wavesDescriptorSetUpdater(wavesDescriptorSet);
        wavesDescriptorSetUpdater.update(device);
        createDescriptorSet(wavesDescriptorSetLayout, descriptorPool, wavesDescriptorSet);

    }

private:
    void createGraphicsPipeline() {
        char const *vert_shader_path = "Assets/Shaders/Vert/terrain.spv";
        char const *frag_shader_path = "Assets/Shaders/Frag/terrain.spv";

        auto vertShaderCode = readFileIntoMem(vert_shader_path);
        auto fragShaderCode = readFileIntoMem(frag_shader_path);

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
        std::array<VkVertexInputAttributeDescription, Vertex::NumBindingLocations> attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // or VK_CULL_MODE_NONE; for no culling
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &actorsDescriptorSetLayout;

        VK_CALL(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        VK_CALL(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline));

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    // each mesh has its own descriptor set and we need to allocate
    // a descriptor for each item in the set.
    void createDescriptorPool(uint32_t numMeshes) {
        std::array<VkDescriptorPoolSize, 3> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * numMeshes);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * numMeshes);
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * numMeshes);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * numMeshes);

        VK_CALL(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool));
    }

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

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

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
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &res.descriptorSet, 0, nullptr);
            vkCmdDrawIndexed(commandBuffer, meshRenderInfo.meshRef.indexCount, (uint32_t)meshRenderInfo.actors.size(), meshRenderInfo.meshRef.firstIndex, meshRenderInfo.meshRef.firstVertex, 0);
        }

        // vkCmdBindIndexBuffer(commandBuffer, wavesRender.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        // vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &wavesDescriptorSet, 0, nullptr);
        // vkCmdDrawIndexed(commandBuffer, (uint32_t)waves.indices.size(), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        VK_CALL(vkEndCommandBuffer(commandBuffer));
    }

    void cleanup() override {
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

        for (auto ubo: ubos) {
            vkDestroyBuffer(device, ubo.buf, nullptr);
            vkFreeMemory(device, ubo.mem, nullptr);
        }

        for (auto &meshActor : meshActors) {
            meshActor.second.cleanup(device);
        }

        vkDestroyDescriptorSetLayout(device, actorsDescriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
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
