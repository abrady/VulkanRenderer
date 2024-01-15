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

struct MeshResources
{
    Vulk &vk;
    VulkMesh mesh;
    VulkTextureView textureView;
    VulkTextureView normalView;

    MeshResources(Vulk &vk, VulkMesh &&mesh, std::string const &texturePath, std::string const &normalPath)
        : vk(vk),
          mesh(std::move(mesh)),
          textureView(vk, texturePath),
          normalView(vk, normalPath) {}
};

struct DescriptorSetInfo
{
    Vulk &vk;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

    DescriptorSetInfo(Vulk &vk, VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> &&descriptorSets)
        : vk(vk),
          descriptorSetLayout(descriptorSetLayout),
          descriptorPool(descriptorPool),
          descriptorSets(std::move(descriptorSets)) {}

    ~DescriptorSetInfo()
    {
        vkDestroyDescriptorPool(vk.device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(vk.device, descriptorSetLayout, nullptr);
    }
};

class DescriptorSetInfoBuilder
{
    Vulk &vk;
    VulkDescriptorSetLayoutBuilder layoutBuilder;
    VulkDescriptorPoolBuilder poolBuilder;
    struct BufSetUpdaterInfo
    {
        VkBuffer buf;
        VkDeviceSize range;
    };
    std::unordered_map<VulkShaderUBOBindings, BufSetUpdaterInfo> uniformSetInfos;
    std::unordered_map<VulkShaderSSBOBindings, BufSetUpdaterInfo> ssboSetInfos;

    struct SamplerSetUpdaterInfo
    {
        VkImageView imageView;
        VkSampler sampler;
    };
    std::unordered_map<VulkShaderTextureBindings, SamplerSetUpdaterInfo> samplerSetInfos;

public:
    DescriptorSetInfoBuilder(Vulk &vk) : vk(vk), layoutBuilder(vk), poolBuilder(vk) {}

    DescriptorSetInfoBuilder &addUniformBuffer(VkShaderStageFlags stageFlags, VulkShaderUBOBindings bindingID, VkBuffer buf, VkDeviceSize range)
    {
        layoutBuilder.addUniformBuffer(bindingID, stageFlags);
        poolBuilder.addUniformBufferCount(MAX_FRAMES_IN_FLIGHT);
        uniformSetInfos[bindingID] = {buf, range};
        return *this;
    }

    DescriptorSetInfoBuilder &addStorageBuffer(VkShaderStageFlags stageFlags, VulkShaderSSBOBindings bindingID, VkBuffer buf, VkDeviceSize range)
    {
        layoutBuilder.addStorageBuffer(bindingID, stageFlags);
        poolBuilder.addStorageBufferCount(MAX_FRAMES_IN_FLIGHT);
        ssboSetInfos[bindingID] = {buf, range};
        return *this;
    }

    DescriptorSetInfoBuilder &addImageSampler(VkShaderStageFlags stageFlags, VulkShaderTextureBindings bindingID, VkImageView imageView, VkSampler sampler)
    {
        layoutBuilder.addSampler(bindingID, stageFlags);
        poolBuilder.addCombinedImageSamplerCount(MAX_FRAMES_IN_FLIGHT);
        samplerSetInfos[bindingID] = {imageView, sampler};
        return *this;
    }

    std::unique_ptr<DescriptorSetInfo> build()
    {
        VkDescriptorSetLayout descriptorSetLayout = layoutBuilder.build();
        VkDescriptorPool pool = poolBuilder.build(MAX_FRAMES_IN_FLIGHT);
        std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            descriptorSets[i] = vk.createDescriptorSet(descriptorSetLayout, pool);
            VulkDescriptorSetUpdater updater = VulkDescriptorSetUpdater(descriptorSets[i]);

            for (auto &pair : uniformSetInfos)
            {
                updater.addUniformBuffer(pair.second.buf, pair.second.range, pair.first);
            }
            for (auto &pair : ssboSetInfos)
            {
                updater.addStorageBuffer(pair.second.buf, pair.second.range, pair.first);
            }
            for (auto &pair : samplerSetInfos)
            {
                updater.addImageSampler(pair.second.imageView, pair.second.sampler, pair.first);
            }

            updater.update(vk.device);
        }
        return std::make_unique<DescriptorSetInfo>(vk, descriptorSetLayout, pool, std::move(descriptorSets));
    }
};

class Renderable
{
    std::unique_ptr<DescriptorSetInfo> dsInfo;
    std::unique_ptr<VulkPipeline> plInfo;
    Vulk &vk;
    uint32_t numIndices;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

public:
    Renderable(Vulk &vk, VulkMesh const &mesh, std::unique_ptr<DescriptorSetInfo> &&dsInfo, std::unique_ptr<VulkPipeline> &&plInfo)
        : vk(vk),
          numIndices((uint32_t)mesh.indices.size()),
          dsInfo(std::move(dsInfo)),
          plInfo(std::move(plInfo))

    {
        auto &vertices = mesh.vertices;
        auto &indices = mesh.indices;
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        vk.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
        bufferSize = sizeof(indices[0]) * indices.size();
        vk.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

        bufferSize = sizeof(vertices[0]) * vertices.size();
        vk.copyFromMemToBuffer(vertices.data(), vertexBuffer, bufferSize);

        bufferSize = sizeof(indices[0]) * indices.size();
        vk.copyFromMemToBuffer(indices.data(), indexBuffer, bufferSize);
    }

    void render(VkCommandBuffer commandBuffer, uint32_t currentFrame, VkViewport const &viewport, VkRect2D const &scissor)
    {
        VkDeviceSize offsets[] = {0};

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, plInfo->pipeline);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, plInfo->pipelineLayout, 0, 1, &dsInfo->descriptorSets[currentFrame], 0, nullptr);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, numIndices, 1, 0, 0, 0);
    }

    ~Renderable()
    {
        vkDestroyBuffer(vk.device, vertexBuffer, nullptr);
        vkFreeMemory(vk.device, vertexBufferMemory, nullptr);
        vkDestroyBuffer(vk.device, indexBuffer, nullptr);
        vkFreeMemory(vk.device, indexBufferMemory, nullptr);
    }
};

class MirrorWorld
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

    struct ModelUBOs
    {
        Vulk &vk;
        std::array<VulkUniformBuffer<XformsUBO>, MAX_FRAMES_IN_FLIGHT> xforms;
        std::array<VulkUniformBuffer<glm::vec3>, MAX_FRAMES_IN_FLIGHT> eyePos;
        VulkUniformBuffer<VulkLight> light;
        ModelUBOs(Vulk &vk) : vk(vk)
        {
            for (auto &ubo : xforms)
            {
                ubo.init(vk);
            }
            for (auto &ubo : eyePos)
            {
                ubo.init(vk);
            }
            light.init(vk);
        }
        ~ModelUBOs()
        {
            for (auto &ubo : xforms)
            {
                ubo.cleanup(vk);
            }
            for (auto &ubo : eyePos)
            {
                ubo.cleanup(vk);
            }
            light.cleanup(vk);
        }
    };
    ModelUBOs modelUBOs;

    VkSampler textureSampler;
    std::unique_ptr<MeshResources> wallResources, skullResources;

    std::unique_ptr<Renderable> wallRenderable, skullRenderable;

public:
    MirrorWorld(Vulk &vk) : vk(vk),
                            modelUBOs(vk)
    {
        // load resources

        VulkMesh wallMesh;
        makeQuad(2.0f, 2.0f, 1, wallMesh);
        wallMesh.xform(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.f)));

        wallResources = std::make_unique<MeshResources>(vk, std::move(wallMesh), "Assets/Textures/RedBrick/RedBrick.jpg", "Assets/Textures/RedBrick/RedBrickNormal.jpg");
        skullResources = std::make_unique<MeshResources>(vk, VulkMesh::loadFromFile("Assets/Models/Skull/Skull.obj"), "Assets/Models/Skull/DiffuseMap.png", "Assets/Models/Skull/NormalMap.png");

        // set up render globals

        modelUBOs.light.mappedUBO->pos = glm::vec3(2.0f, .5f, .5f);
        modelUBOs.light.mappedUBO->color = glm::vec3(.7f, .7f, .7f);
        camera.lookAt(glm::vec3(0.0f, 0.f, 1.3f), glm::vec3(0.f, 0.f, 0.f));

        textureSampler = vk.createTextureSampler();

        // set up descriptors

        DescriptorSetInfoBuilder dsBuilder = DescriptorSetInfoBuilder(vk)
                                                 .addUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, VulkShaderUBOBinding_Xforms, modelUBOs.xforms[0].buf, modelUBOs.xforms[0].getSize())
                                                 .addUniformBuffer(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderUBOBinding_EyePos, modelUBOs.eyePos[0].buf, modelUBOs.eyePos[0].getSize())
                                                 .addUniformBuffer(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderUBOBinding_Lights, modelUBOs.light.buf, modelUBOs.light.getSize());

        std::unique_ptr<DescriptorSetInfo> skullDS = DescriptorSetInfoBuilder(dsBuilder)
                                                         .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_TextureSampler, skullResources->textureView.textureImageView, textureSampler)
                                                         .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_NormalSampler, skullResources->normalView.textureImageView, textureSampler)
                                                         .build();
        std::unique_ptr<DescriptorSetInfo> wallDS = DescriptorSetInfoBuilder(dsBuilder)
                                                        .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_TextureSampler, wallResources->textureView.textureImageView, textureSampler)
                                                        .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_NormalSampler, wallResources->normalView.textureImageView, textureSampler)
                                                        .build();

        // and finally the pipelines
        std::unique_ptr<VulkPipeline> skullPipeline = VulkPipelineBuilder(vk)
                                                          .addVertexShaderStage("Source/Shaders/Vert/LitModel.spv")
                                                          .addFragmentShaderStage("Source/Shaders/Frag/LitModel.spv")
                                                          .addVulkVertexInput(0)
                                                          .build(skullDS->descriptorSetLayout);
        std::unique_ptr<VulkPipeline> wallPipeline = VulkPipelineBuilder(vk)
                                                         .addVertexShaderStage("Source/Shaders/Vert/LitModel.spv")
                                                         .addFragmentShaderStage("Source/Shaders/Frag/LitModel.spv")
                                                         .addVulkVertexInput(0)
                                                         .build(wallDS->descriptorSetLayout);

        // pass ownership of everything off to the renderable.
        wallRenderable = std::make_unique<Renderable>(vk, wallResources->mesh, std::move(wallDS), std::move(wallPipeline));
        skullRenderable = std::make_unique<Renderable>(vk, skullResources->mesh, std::move(skullDS), std::move(skullPipeline));
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
        updateXformsUBO(*modelUBOs.xforms[currentFrame].mappedUBO, viewport);
        *modelUBOs.eyePos[currentFrame].mappedUBO = camera.eye;

        skullRenderable->render(commandBuffer, currentFrame, viewport, scissor);
        wallRenderable->render(commandBuffer, currentFrame, viewport, scissor);
    }

    ~MirrorWorld()
    {
        vkDestroySampler(vk.device, textureSampler, nullptr);
    }
};
