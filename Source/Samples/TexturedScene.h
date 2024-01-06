#pragma once

#include "Vulk/Vulk.h"
#include "Vulk/VulkGeo.h"
#include "Vulk/VulkActor.h"
#include "Vulk/VulkCamera.h"
#include "Vulk/VulkPipelineBuilder.h"
#include "Vulk/VulkDescriptorPoolBuilder.h"
#include "Vulk/VulkUniformBuffer.h"
#include "Vulk/VulkStorageBuffer.h"

class TexturedScene : public Vulk {
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

        // render-related
        VkDescriptorPool descriptorPool;
        std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets; // the per-frame descriptor sets
        std::array<VulkStorageBuffer<ActorSSBOElt>, MAX_FRAMES_IN_FLIGHT> ssbos; // the per-frame actor xforms

        // for debugging
        VkDescriptorPool normalsDescriptorPool;
        std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> normalsDescriptorSets = {}; 

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
            vkDestroyDescriptorPool(dev, descriptorPool, nullptr);
            vkDestroyDescriptorPool(dev, normalsDescriptorPool, nullptr);
        }
    };
    
    std::unordered_map<char const *, MeshRenderInfo> meshActors;

    struct MeshRenderBuffers {
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
    MeshRenderBuffers actorsRenderBuffers;
    VkPipelineLayout actorsPipelineLayout;
    VkPipeline actorsGraphicsPipeline;

    VkPipelineLayout wavesPipelineLayout;
    VkPipeline wavesGraphicsPipeline;
    VkDescriptorPool wavesDescriptorPool;
    VkDescriptorSetLayout wavesDescriptorSetLayout;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> wavesDescriptorSets;
    VulkMesh wavesMesh;
    std::array<MeshRenderBuffers, MAX_FRAMES_IN_FLIGHT> wavesRender;

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

    struct TextureView {
        VkImage textureImage;
        VkDeviceMemory textureImageMemory;
        VkImageView textureImageView;

        void init(Vulk &vk, char const *texturePath) {
            vk.createTextureImage(texturePath, textureImageMemory, textureImage);
            textureImageView = vk.createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
        }

        void cleanup(Vulk &vk) {
            vkDestroyImageView(vk.device, textureImageView, nullptr);
            vkDestroyImage(vk.device, textureImage, nullptr);
            vkFreeMemory(vk.device, textureImageMemory, nullptr);
        }
    };

    TextureView beachTextureView, grassTextureView, snowTextureView, wavesTextureView;
    VkSampler textureSampler;

    bool rotateWorld = true;

    struct Material {
        glm::vec4 diffuse;
        glm::vec3 fresnelR0;
        float roughness;
    };
    Material terrainMaterial = {
        {0.9f, 0.9f, 0.9f, 1.0f},
        {0.05f, 0.05f, 0.05f},
        1.0f
    };
    VulkStorageBuffer<Material> terrainSSBO;

    Material wavesMaterial = {
        {0.9f, 0.9f, 0.9f, .5f},
        {0.02f, 0.02f, 0.02f},
        0.5f
    };
    VulkStorageBuffer<Material> wavesSSBO;

    struct Light {
        glm::vec3 pos;          // point light only
        float falloffStart;     // point/spot light only
        glm::vec3 color;        // color of light
        float falloffEnd;       // point/spot light only: negative means no falloff 
        glm::vec3 direction;    // directional/spot light only
        float spotPower;        // spotlight only
    };
    Light light = {
        {600.0f, 300.0f, 0.0f},
        0.0f,
        {.7f, .7f, .5f},
        //{0.f, 0.f, 0.f},
        0.0f,
        {0.0f, 0.0f, 0.0f},
        0.0f,
    };
    VulkStorageBuffer<Light> lightSSBO;

    float getTerrainHeight(Vertex const &v) {
        return 0.3f * (v.pos.z * sinf(0.1f * v.pos.x) + v.pos.x * cosf(0.1f * v.pos.z));
    }

    glm::vec3 getTerrainNormal(Vertex const &v) {
        float x = v.pos.x;
        float z = v.pos.z;

        // given the grad of our y function (dy/dx,dy/dz) we can find two tangent vectors as follows:
        // * Tx = (1,dy/dx,0)
        // * Tz = (0,dy/dz,1)
        //
        // The cross product of Tz x Tx is the normal vector in the positive y direction:
        // * (-dy/dx,1,dz/dx)

        // given the terrain height function: 0.3f * (v.pos.z * sinf(0.1f * v.pos.x) + v.pos.x * cosf(0.1f * v.pos.z));
        float dydx = 0.3f * (0.1f * z * cosf(0.1f * x) + cosf(0.1f * z));
        float dydz = 0.3f * (sinf(0.1f * x) - 0.1f * x * sinf(0.1f * z));
        glm::vec3 n = {-dydx, 1, -dydz};
        return glm::normalize(n);
    }

    bool renderNormals = false;
    bool renderWaves = true;
    VkPipelineLayout normalsPipelineLayout;
    VkDescriptorSetLayout normalsDescriptorSetLayout;
    VkPipeline normalsPipeline;
public:
    void init() override {
        camera.lookAt(glm::vec3(15.f, 120.f, 170.f), glm::vec3(0.f, 0.f, 0.f));

        beachTextureView.init(*this, "Assets/Textures/aerial_beach_01/aerial_beach_01_diff_4k.jpg");
        grassTextureView.init(*this, "Assets/Textures/aerial_rocks_02/aerial_rocks_02_diff_4k.jpg");
        snowTextureView.init(*this, "Assets/Textures/snow_02/snow_02_diff_4k.jpg");
        textureSampler = createTextureSampler();

        wavesTextureView.init(*this, "Assets/Textures/sea_water_2048x2048.png");

        for (auto &ubo: xformsUBOs) {
            ubo.createUniformBuffers(*this);
        }
        for (auto &ubo: eyePosUBOs) {
            ubo.createUniformBuffers(*this);
        }

        // create the materials SSBO
        terrainSSBO.createAndMap(*this, 1);
        terrainSSBO.mappedObjs[0] = terrainMaterial;
        wavesSSBO.createAndMap(*this, 1);
        wavesSSBO.mappedObjs[0] = wavesMaterial;

        // create the lights SSBO
        lightSSBO.createAndMap(*this, 1);
        lightSSBO.mappedObjs[0] = light;

        actorsDescriptorSetLayout = VulkDescriptorSetLayoutBuilder()
            .addUniformBuffer(VulkShaderBinding_XformsUBO, VK_SHADER_STAGE_VERTEX_BIT)
            .addUniformBuffer(VulkShaderBinding_EyePos, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addSampler(VulkShaderBinding_TextureSampler)
            .addSampler(VulkShaderBinding_TextureSampler2)
            .addSampler(VulkShaderBinding_TextureSampler3)
            .addStorageBuffer(VulkShaderBinding_Actors, VK_SHADER_STAGE_VERTEX_BIT)
            .addStorageBuffer(VulkShaderBinding_Lights, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addStorageBuffer(VulkShaderBinding_Materials, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build(*this);

        VulkPipelineBuilder(*this)
            .addVertexShaderStage("Assets/Shaders/Vert/litTexturedTerrain.spv")
            .addVertexInputBindingDescription(0, sizeof(Vertex))
            .addVertexInputFieldVec3(0, Vertex::PosBinding, offsetof(Vertex, pos))
            .addVertexInputFieldVec3(0, Vertex::NormalBinding, offsetof(Vertex, normal))
            .addVertexInputFieldVec3(0, Vertex::TangentBinding, offsetof(Vertex, tangent))
            .addVertexInputFieldVec2(0, Vertex::TexCoordBinding, offsetof(Vertex, texCoord))
            .addFragmentShaderStage("Assets/Shaders/Frag/litTexturedTerrain.spv")
            .build(actorsDescriptorSetLayout, &actorsPipelineLayout, &actorsGraphicsPipeline);

        // test normal rendering: do something a little simpler
        // camera.lookAt(glm::vec3(0.f, 0.f, 2.1f), glm::vec3(0.f, 0.f, 0.f));
        // VulkMesh sphere;
        // makeGeoSphere(1.f, 3, sphere);
        // meshActors["sphere"] = {
        //     meshAccumulator.appendMesh(sphere),
        //     {{"sphere0", glm::translate(glm::mat4(1.0f), glm::vec3(0.f, 0.f, 0.f))}}
        // };

        VulkMesh terrain;
        makeGrid(160, 160, 50, 50, terrain, 4, 4);
        for (auto &v : terrain.vertices) {
            v.pos.y = getTerrainHeight(v);
            v.normal = getTerrainNormal(v);
        }

        VulkMeshRef terrainRef = meshAccumulator.appendMesh(terrain);
        meshActors["terrain"] = {
            meshAccumulator.appendMesh(terrain),
            {{"terrain0", glm::translate(glm::mat4(1.0f), glm::vec3(0.f, 0.f, 0.f))}}
        };

        actorsRenderBuffers.init(*this, meshAccumulator.vertices, meshAccumulator.indices);

        // for debugging
        normalsDescriptorSetLayout = VulkDescriptorSetLayoutBuilder()
            .addUniformBuffer(VulkShaderBinding_XformsUBO, VK_SHADER_STAGE_GEOMETRY_BIT)
            .addStorageBuffer(VulkShaderBinding_Actors, VK_SHADER_STAGE_GEOMETRY_BIT)
            .build(*this);

        VulkPipelineBuilder(*this)
            .addVertexShaderStage("Assets/Shaders/Vert/normals.spv") 
            .setPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
            .addGeometryShaderStage("Assets/Shaders/Geom/normals.spv") 
            .addVertexInputBindingDescription(0, sizeof(Vertex))
            .addVertexInputFieldVec3(0, Vertex::PosBinding, offsetof(Vertex, pos))
            .addVertexInputFieldVec3(0, Vertex::NormalBinding, offsetof(Vertex, normal))
            .addVertexInputFieldVec3(0, Vertex::TangentBinding, offsetof(Vertex, tangent))
            .addVertexInputFieldVec2(0, Vertex::TexCoordBinding, offsetof(Vertex, texCoord))
            .addFragmentShaderStage("Assets/Shaders/Frag/normals.spv")
            // .setLineWidth(10.f)
            .setCullMode(VK_CULL_MODE_NONE)
            // .setDepthTestEnabled(false)
            .setDepthWriteEnabled(false)
            .build(normalsDescriptorSetLayout, &normalsPipelineLayout, &normalsPipeline);        

        for (auto &meshActor : meshActors) {
            auto &meshRenderInfo = meshActor.second;
            uint32_t numActors = static_cast<uint32_t>(meshRenderInfo.actors.size());
            meshRenderInfo.descriptorPool = VulkDescriptorPoolBuilder()
                .addUniformBufferCount(MAX_FRAMES_IN_FLIGHT * 2)
                .addCombinedImageSamplerCount(MAX_FRAMES_IN_FLIGHT * 3)
                .addStorageBufferCount(MAX_FRAMES_IN_FLIGHT * 3)
                .build(device, MAX_FRAMES_IN_FLIGHT);

            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                // map the actor xforms into a mem-mapped SSBO
                meshRenderInfo.ssbos[i].createAndMap(*this, numActors);

                // create the descriptor set
                meshRenderInfo.descriptorSets[i] = createDescriptorSet(actorsDescriptorSetLayout, meshRenderInfo.descriptorPool);
                VulkDescriptorSetUpdater(meshRenderInfo.descriptorSets[i])
                    .addUniformBuffer(xformsUBOs[i].buf, xformsUBOs[i].getSize(), VulkShaderBinding_XformsUBO)
                    .addUniformBuffer(eyePosUBOs[i].buf, eyePosUBOs[i].getSize(), VulkShaderBinding_EyePos)
                    .addImageSampler(beachTextureView.textureImageView, textureSampler, VulkShaderBinding_TextureSampler)
                    .addImageSampler(grassTextureView.textureImageView, textureSampler, VulkShaderBinding_TextureSampler2)
                    .addImageSampler(snowTextureView.textureImageView, textureSampler, VulkShaderBinding_TextureSampler3)
                    .addStorageBuffer(meshRenderInfo.ssbos[i].buf, sizeof(ActorSSBOElt) * numActors, VulkShaderBinding_Actors)
                    .addStorageBuffer(lightSSBO.buf, sizeof(Light), VulkShaderBinding_Lights)
                    .addStorageBuffer(terrainSSBO.buf, sizeof(Material), VulkShaderBinding_Materials)
                    .update(device);
            }

            // for debugging
            meshRenderInfo.normalsDescriptorPool = VulkDescriptorPoolBuilder()
                .addUniformBufferCount(MAX_FRAMES_IN_FLIGHT)
                .addStorageBufferCount(MAX_FRAMES_IN_FLIGHT)
                .build(device, MAX_FRAMES_IN_FLIGHT);

            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                meshRenderInfo.normalsDescriptorSets[i] = createDescriptorSet(normalsDescriptorSetLayout, meshRenderInfo.normalsDescriptorPool);
                VulkDescriptorSetUpdater(meshRenderInfo.normalsDescriptorSets[i])
                    .addUniformBuffer(xformsUBOs[i].buf, xformsUBOs[i].getSize(), VulkShaderBinding_XformsUBO)
                    .addStorageBuffer(meshRenderInfo.ssbos[i].buf, sizeof(ActorSSBOElt) * numActors, VulkShaderBinding_Actors)
                    .update(device);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // waves
        
        makeGrid(160, 160, 50, 50, wavesMesh);
        for (auto &render: wavesRender) {
            render.init(*this, wavesMesh.vertices, wavesMesh.indices);
        }

        wavesDescriptorSetLayout = VulkDescriptorSetLayoutBuilder()
            .addUniformBuffer(VulkShaderBinding_XformsUBO, VK_SHADER_STAGE_VERTEX_BIT)
            .addUniformBuffer(VulkShaderBinding_EyePos, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addSampler(VulkShaderBinding_TextureSampler)
            .addStorageBuffer(VulkShaderBinding_Lights, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addStorageBuffer(VulkShaderBinding_Materials, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build(*this);

        VulkPipelineBuilder(*this)
            .addVertexShaderStage("Assets/Shaders/Vert/litTexturedWaves.spv")
            .addVertexInputBindingDescription(0,sizeof(Vertex))
            .addVertexInputFieldVec3(0, Vertex::PosBinding, offsetof(Vertex, pos))
            .addVertexInputFieldVec3(0, Vertex::NormalBinding, offsetof(Vertex, normal))
            .addVertexInputFieldVec3(0, Vertex::TangentBinding, offsetof(Vertex, tangent))
            .addVertexInputFieldVec2(0, Vertex::TexCoordBinding, offsetof(Vertex, texCoord))
            .addFragmentShaderStage("Assets/Shaders/Frag/litTexturedWaves.spv")
            .setBlendingEnabled(true)
            .build(wavesDescriptorSetLayout, &wavesPipelineLayout, &wavesGraphicsPipeline);

        wavesDescriptorPool = VulkDescriptorPoolBuilder()
            .addUniformBufferCount(MAX_FRAMES_IN_FLIGHT * 2)
            .addCombinedImageSamplerCount(MAX_FRAMES_IN_FLIGHT)
            .addStorageBufferCount(MAX_FRAMES_IN_FLIGHT * 3)
            .build(device, MAX_FRAMES_IN_FLIGHT);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            wavesDescriptorSets[i] = createDescriptorSet(wavesDescriptorSetLayout, wavesDescriptorPool);
            VulkDescriptorSetUpdater(wavesDescriptorSets[i])
                .addUniformBuffer(xformsUBOs[i].buf, xformsUBOs[i].getSize(), VulkShaderBinding_XformsUBO)
                .addUniformBuffer(eyePosUBOs[i].buf, eyePosUBOs[i].getSize(), VulkShaderBinding_EyePos)
                .addImageSampler(wavesTextureView.textureImageView, textureSampler, VulkShaderBinding_TextureSampler)
                .addStorageBuffer(lightSSBO.buf, sizeof(Light), VulkShaderBinding_Lights)
                .addStorageBuffer(wavesSSBO.buf, sizeof(Material), VulkShaderBinding_Materials)
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

    std::chrono::steady_clock::time_point rotateWorldTimerStartTime = std::chrono::high_resolution_clock::now();
    std::chrono::steady_clock::time_point pauseTime = std::chrono::high_resolution_clock::now();
    void updateXformsUBO(XformsUBO &ubo) {
        
        auto currentTime = std::chrono::high_resolution_clock::now();
        if (!rotateWorld) {
            currentTime = pauseTime;
        }
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - rotateWorldTimerStartTime).count();
        ubo.world = glm::rotate(glm::mat4(1.0f), 0.5f * time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec3 fwd = camera.getForwardVec();
        glm::vec3 lookAt = camera.eye + fwd;
        // glm::vec3 up = camera.getUpVec();
        ubo.view = glm::lookAt(camera.eye, lookAt, glm::vec3(0, 1, 0));
        ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 1.f, 4000.0f);
        ubo.proj[1][1] *= -1;
    }

    std::chrono::steady_clock::time_point updateWavesStartTime = std::chrono::high_resolution_clock::now();
    void wavesTick() {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - updateWavesStartTime).count();

        for (auto &v : wavesMesh.vertices) {
            v.pos.y = 0.05f * (v.pos.z * sinf(0.1f * v.pos.x + time) + v.pos.x * cosf(0.1f * v.pos.z + time)) - 10.0f;

            // take instantaneous partial derivatives to get the normal
            float dydx = 0.05f * (0.1f * v.pos.z * cosf(0.1f * v.pos.x + time) + cosf(0.1f * v.pos.z + time));
            float dydz = 0.05f * (sinf(0.1f * v.pos.x + time) - 0.1f * v.pos.x * cosf(0.1f * v.pos.z + time));
            v.normal = glm::normalize(glm::vec3(-dydx, 1, -dydz));
        }

        // update the buffer
        wavesRender[(currentFrame + 1) % MAX_FRAMES_IN_FLIGHT].copyToBuffer(*this, wavesMesh.vertices, wavesMesh.indices);
    }

    void drawFrame(VkCommandBuffer commandBuffer, VkFramebuffer frameBuffer) override {
        wavesTick();
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
        clearValues[0].color = { {0.1f, 0.0f, 0.1f, 1.0f} };
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

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &actorsRenderBuffers.vertexBuffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, actorsRenderBuffers.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        for (auto &meshActor: meshActors) {
            MeshRenderInfo &meshRenderInfo = meshActor.second;
            meshRenderInfo.updateActorSSBO(currentFrame); // is this safe? need to understand synchroniation better...;
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actorsPipelineLayout, 0, 1, &meshRenderInfo.descriptorSets[currentFrame], 0, nullptr);
            vkCmdDrawIndexed(commandBuffer, meshRenderInfo.meshRef.indexCount, (uint32_t)meshRenderInfo.actors.size(), meshRenderInfo.meshRef.firstIndex, meshRenderInfo.meshRef.firstVertex, 0);
        }

        if (renderNormals) {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, normalsPipeline);
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
            // vert and index buffers are not preserved across new pipelines (I think, I should double check)
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &actorsRenderBuffers.vertexBuffer, offsets);
            vkCmdBindIndexBuffer(commandBuffer, actorsRenderBuffers.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            for (auto &meshActor: meshActors) {
                MeshRenderInfo &meshRenderInfo = meshActor.second;
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, normalsPipelineLayout, 0, 1, &meshRenderInfo.normalsDescriptorSets[currentFrame], 0, nullptr);
                vkCmdDrawIndexed(commandBuffer, meshRenderInfo.meshRef.indexCount, (uint32_t)meshRenderInfo.actors.size(), meshRenderInfo.meshRef.firstIndex, meshRenderInfo.meshRef.firstVertex, 0);
            }
        }

        // waves
        if (renderWaves) {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wavesGraphicsPipeline);
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &wavesRender[currentFrame].vertexBuffer, offsets);
            vkCmdBindIndexBuffer(commandBuffer, wavesRender[currentFrame].indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wavesPipelineLayout, 0, 1, &wavesDescriptorSets[currentFrame], 0, nullptr);
            vkCmdDrawIndexed(commandBuffer, (uint32_t)wavesMesh.indices.size(), 1, 0, 0, 0);
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
        beachTextureView.cleanup(*this);
        grassTextureView.cleanup(*this);
        snowTextureView.cleanup(*this);
        wavesTextureView.cleanup(*this);
        vkDestroySampler(device, textureSampler, nullptr);
        actorsRenderBuffers.cleanup(*this);
        terrainSSBO.cleanup(device);
        wavesSSBO.cleanup(device);
        lightSSBO.cleanup(device);

        vkDestroyDescriptorSetLayout(device, wavesDescriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(device, wavesDescriptorPool, nullptr);
        vkDestroyPipeline(device, wavesGraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, wavesPipelineLayout, nullptr);
        for (auto &render : wavesRender) {
            render.cleanup(*this);
        }

        vkDestroyDescriptorSetLayout(device, normalsDescriptorSetLayout, nullptr);
        vkDestroyPipeline(device, normalsPipeline, nullptr);
        vkDestroyPipelineLayout(device, normalsPipelineLayout, nullptr);
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
            } else if (key == GLFW_KEY_SPACE) {
                if (rotateWorld) {
                    pauseTime = std::chrono::high_resolution_clock::now();
                } else {
                    rotateWorldTimerStartTime += std::chrono::high_resolution_clock::now() - pauseTime;
                }
                rotateWorld = !rotateWorld;
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
