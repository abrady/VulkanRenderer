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

    MeshResources(Vulk &vk, VulkMesh &&mesh, std::string const &textureName, std::string const &normalName)
        : vk(vk),
          mesh(std::move(mesh)),
          textureView(vk, VULK_TEXTURE_DIR + textureName),
          normalView(vk, VULK_TEXTURE_DIR + normalName) {}
};

class Resources
{
    Vulk &vk;
    std::unordered_map<std::string, std::unique_ptr<MeshResources>> meshResources;
    std::unordered_map<std::string, VkShaderModule> vertShaders, fragShaders;
    VkSampler textureSampler;

    enum ShaderType
    {
        Vertex,
        Fragment
    };

    VkShaderModule createShaderModule(ShaderType type, std::string const &name)
    {
        std::string subdir;
        switch (type)
        {
        case Vertex:
            subdir = "Vert";
            break;
        case Fragment:
            subdir = "Frag";
            break;
        };

        std::string path = VULK_SHADERS_DIR + subdir + "/" + name + ".spv";
        auto shaderCode = readFileIntoMem(path);
        VkShaderModule shaderModule = vk.createShaderModule(shaderCode);
        return shaderModule;
    }

public:
    Resources(Vulk &vk)
        : vk(vk)
    {
        textureSampler = vk.createTextureSampler();
    }

    Resources &addMesh(std::string const &name, std::unique_ptr<MeshResources> &&mesh)
    {
        ASSERT_KEY_NOT_SET(meshResources, name);
        meshResources[name] = std::move(mesh);
        return *this;
    }

    Resources &loadVertexShader(std::string name)
    {
        ASSERT_KEY_NOT_SET(vertShaders, name);
        VkShaderModule shaderModule = createShaderModule(Vertex, name);
        vertShaders[name] = shaderModule;
        return *this;
    }

    Resources &loadFragmentShader(std::string name)
    {
        ASSERT_KEY_NOT_SET(fragShaders, name);
        VkShaderModule shaderModule = createShaderModule(Fragment, name);
        fragShaders[name] = shaderModule;
        return *this;
    }

    VkShaderModule getVertexShader(std::string const &name)
    {
        if (vertShaders.find(name) == vertShaders.end())
        {
            loadVertexShader(name);
        }
        ASSERT_KEY_SET(vertShaders, name);
        return vertShaders[name];
    }

    VkShaderModule getFragmentShader(std::string const &name)
    {
        if (fragShaders.find(name) == fragShaders.end())
        {
            loadFragmentShader(name);
        }
        ASSERT_KEY_SET(fragShaders, name);
        return fragShaders[name];
    }

    MeshResources &getMesh(std::string const &name)
    {
        ASSERT_KEY_SET(meshResources, name);
        return *meshResources[name];
    }

    VkSampler getTextureSampler()
    {
        return textureSampler;
    }

    ~Resources()
    {
        for (auto &pair : vertShaders)
        {
            vkDestroyShaderModule(vk.device, pair.second, nullptr);
        }
        for (auto &pair : fragShaders)
        {
            vkDestroyShaderModule(vk.device, pair.second, nullptr);
        }
        vkDestroySampler(vk.device, textureSampler, nullptr);
    }
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

    struct PerFrameInfo
    {
        std::unordered_map<VulkShaderUBOBindings, BufSetUpdaterInfo> uniformSetInfos;
        std::unordered_map<VulkShaderSSBOBindings, BufSetUpdaterInfo> ssboSetInfos;
    };
    std::array<PerFrameInfo, MAX_FRAMES_IN_FLIGHT> perFrameInfos;

    struct SamplerSetUpdaterInfo
    {
        VkImageView imageView;
        VkSampler sampler;
    };
    std::unordered_map<VulkShaderTextureBindings, SamplerSetUpdaterInfo> samplerSetInfos;

public:
    DescriptorSetInfoBuilder(Vulk &vk) : vk(vk), layoutBuilder(vk), poolBuilder(vk) {}

    template <typename T>
    DescriptorSetInfoBuilder &addUniformBuffers(std::array<VulkUniformBuffer<T>, MAX_FRAMES_IN_FLIGHT> &uniformBuffers, VkShaderStageFlags stageFlags, VulkShaderUBOBindings bindingID)
    {
        layoutBuilder.addUniformBuffer(bindingID, stageFlags);
        poolBuilder.addUniformBufferCount(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            perFrameInfos[i].uniformSetInfos[bindingID] = {uniformBuffers[i].buf, sizeof(T)};
        }
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

            for (auto &pair : perFrameInfos[i].uniformSetInfos)
            {
                updater.addUniformBuffer(pair.second.buf, pair.second.range, pair.first);
            }
            for (auto &pair : perFrameInfos[i].ssboSetInfos)
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

class RenderState
{
    std::unique_ptr<DescriptorSetInfo> dsInfo;
    std::unique_ptr<VulkPipeline> plInfo;
    Vulk &vk;

public:
    RenderState(Vulk &vk, std::unique_ptr<DescriptorSetInfo> &&dsInfo, std::unique_ptr<VulkPipeline> &&plInfo)
        : vk(vk),
          dsInfo(std::move(dsInfo)),
          plInfo(std::move(plInfo)) {}

    void setRenderState(VkCommandBuffer commandBuffer, uint32_t currentFrame, VkViewport const &viewport, VkRect2D const &scissor)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, plInfo->pipeline);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, plInfo->pipelineLayout, 0, 1, &dsInfo->descriptorSets[currentFrame], 0, nullptr);
    }
};

class MeshRenderable
{
    Vulk &vk;
    RenderState renderState;
    uint32_t numIndices;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

public:
    MeshRenderable(Vulk &vk, VulkMesh const &mesh, std::unique_ptr<DescriptorSetInfo> &&dsInfo, std::unique_ptr<VulkPipeline> &&plInfo)
        : vk(vk),
          renderState(vk, std::move(dsInfo), std::move(plInfo)),
          numIndices((uint32_t)mesh.indices.size())
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
        renderState.setRenderState(commandBuffer, currentFrame, viewport, scissor);
        bindAndDrawMesh(commandBuffer);
    }

    void bindAndDrawMesh(VkCommandBuffer commandBuffer)
    {
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, numIndices, 1, 0, 0, 0);
    }

    ~MeshRenderable()
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

    template <typename T>
    class FrameUBOs
    {
        Vulk &vk;
        std::array<VulkUniformBuffer<T>, MAX_FRAMES_IN_FLIGHT> ubos;

    public:
        explicit FrameUBOs(Vulk &vk) : vk(vk)
        {
            for (auto &ubo : ubos)
            {
                ubo.init(vk);
            }
        }
        ~FrameUBOs()
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

    struct ModelUBOs
    {
        FrameUBOs<XformsUBO> xforms;
        FrameUBOs<glm::vec3> eyePos;
        FrameUBOs<VulkLight> lights;
        ModelUBOs(Vulk &vk) : xforms(vk), eyePos(vk), lights(vk) {}
    };
    ModelUBOs modelUBOs;

    struct MirrorPlane
    {
        glm::vec4 normal;
        glm::vec4 point;
    };
    FrameUBOs<MirrorPlane> mirroredPlanes;

    Resources resources;
    std::unique_ptr<RenderState> mirrorWorldRenderState, mirrorStencilRenderState;
    std::unique_ptr<MeshRenderable> wallRenderable, skullRenderable, mirrorRenderable;

public:
    MirrorWorld(Vulk &vk) : vk(vk),
                            modelUBOs(vk),
                            resources(vk),
                            mirroredPlanes(vk)
    {
        // load resources
        loadResources();

        // set up render globals
        for (auto &light : modelUBOs.lights)
        {
            light.mappedUBO->pos = glm::vec3(0.0f, 0.0f, 0.0f);
            light.mappedUBO->color = glm::vec3(1.0f, 1.0f, 1.0f);
        }
        camera.lookAt(glm::vec3(.9f, 1.f, 1.3f), glm::vec3(0.5f, 0.f, 0.f));

        // set up mirror plane
        VulkMesh mirrorMesh = resources.getMesh("mirror").mesh;
        glm::vec3 p0 = mirrorMesh.vertices[mirrorMesh.indices[0]].pos;
        glm::vec3 p1 = mirrorMesh.vertices[mirrorMesh.indices[1]].pos;
        glm::vec3 p2 = mirrorMesh.vertices[mirrorMesh.indices[2]].pos;
        glm::vec3 normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
        for (auto &plane : mirroredPlanes)
        {
            plane.mappedUBO->normal = glm::vec4(normal, 0.0f);
            plane.mappedUBO->point = glm::vec4(p0, 1.0f);
        }

        // set up descriptors
        DescriptorSetInfoBuilder dsBuilder = DescriptorSetInfoBuilder(vk)
                                                 .addUniformBuffers(modelUBOs.xforms.getUBOs(), VK_SHADER_STAGE_VERTEX_BIT, VulkShaderUBOBinding_Xforms)
                                                 .addUniformBuffers(modelUBOs.eyePos.getUBOs(), VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderUBOBinding_EyePos)
                                                 .addUniformBuffers(modelUBOs.lights.getUBOs(), VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderUBOBinding_Lights);

        std::unique_ptr<DescriptorSetInfo> skullDS = DescriptorSetInfoBuilder(dsBuilder)
                                                         .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_TextureSampler, resources.getMesh("skull").textureView.textureImageView, resources.getTextureSampler())
                                                         .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_NormalSampler, resources.getMesh("skull").normalView.textureImageView, resources.getTextureSampler())
                                                         .build();
        std::unique_ptr<DescriptorSetInfo> wallDS = DescriptorSetInfoBuilder(dsBuilder)
                                                        .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_TextureSampler, resources.getMesh("wall").textureView.textureImageView, resources.getTextureSampler())
                                                        .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_NormalSampler, resources.getMesh("wall").normalView.textureImageView, resources.getTextureSampler())
                                                        .build();

        std::unique_ptr<DescriptorSetInfo> mirrorDS = DescriptorSetInfoBuilder(dsBuilder)
                                                          .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_TextureSampler, resources.getMesh("mirror").textureView.textureImageView, resources.getTextureSampler())
                                                          .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_NormalSampler, resources.getMesh("mirror").normalView.textureImageView, resources.getTextureSampler())
                                                          .build();

        std::unique_ptr<DescriptorSetInfo> mirrorStencilDS = DescriptorSetInfoBuilder(dsBuilder)
                                                                 .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_TextureSampler, resources.getMesh("mirror").textureView.textureImageView, resources.getTextureSampler())
                                                                 .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_NormalSampler, resources.getMesh("mirror").normalView.textureImageView, resources.getTextureSampler())
                                                                 .build();

        std::unique_ptr<DescriptorSetInfo> mirrorWorldRenderStateDS = DescriptorSetInfoBuilder(dsBuilder)
                                                                          .addUniformBuffers(mirroredPlanes.getUBOs(), VK_SHADER_STAGE_VERTEX_BIT, VulkShaderUBOBinding_MirrorPlaneUBO)
                                                                          .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_TextureSampler, resources.getMesh("skull").textureView.textureImageView, resources.getTextureSampler())
                                                                          .addImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, VulkShaderTextureBinding_NormalSampler, resources.getMesh("skull").normalView.textureImageView, resources.getTextureSampler())
                                                                          .build();

        // and finally the pipelines
        std::unique_ptr<VulkPipeline> skullPipeline = VulkPipelineBuilder(vk)
                                                          .addVertexShaderStage(resources.getVertexShader("LitModel"))
                                                          .addFragmentShaderStage(resources.getFragmentShader("LitModel"))
                                                          .addVulkVertexInput(0)
                                                          .build(skullDS->descriptorSetLayout);
        std::unique_ptr<VulkPipeline> wallPipeline = VulkPipelineBuilder(vk)
                                                         .addVertexShaderStage(resources.getVertexShader("LitModel"))
                                                         .addFragmentShaderStage(resources.getFragmentShader("LitModel"))
                                                         .addVulkVertexInput(0)
                                                         .build(wallDS->descriptorSetLayout);
        std::unique_ptr<VulkPipeline> mirrorPipeline = VulkPipelineBuilder(vk)
                                                           .addVertexShaderStage(resources.getVertexShader("LitModel"))
                                                           .addFragmentShaderStage(resources.getFragmentShader("LitMirror"))
                                                           .addVulkVertexInput(0)
                                                           .build(mirrorDS->descriptorSetLayout);

        // TODO: I should make a special pipeline for the stencil that just looks at verts, but this is fine for now.
        std::unique_ptr<VulkPipeline> mirrorStencilPipeline = VulkPipelineBuilder(vk)
                                                                  .addVertexShaderStage(resources.getVertexShader("LitModel"))
                                                                  .addFragmentShaderStage(resources.getFragmentShader("LitMirror"))
                                                                  .addVulkVertexInput(0)
                                                                  .setBlendingEnabled(true)                       // we don't want to write to the color buffer, just the stencil buffer
                                                                  .setStencilTestEnabled(true)                    // enable stencil testing
                                                                  .setStencilFrontFailOp(VK_STENCIL_OP_KEEP)      // keep the stencil buffer unchanged on fail
                                                                  .setFrontStencilPassOp(VK_STENCIL_OP_REPLACE)   // replace the stencil buffer with the reference value
                                                                  .setFrontStencilCompareOp(VK_COMPARE_OP_ALWAYS) // always pass the stencil test for our first pass
                                                                  .setFrontStencilCompareMask(0xFF)               // compare against all bits
                                                                  .setFrontStencilWriteMask(0xFF)                 // allow write to all bits
                                                                  .setFrontStencilReference(1)                    // value to write to the stencil buffer
                                                                  .build(mirrorStencilDS->descriptorSetLayout);

        // triangle winding order is backwards for the mirror, so I need to flip the cull mode.
        std::unique_ptr<VulkPipeline> mirroredWorldPipeline = VulkPipelineBuilder(vk)
                                                                  .addVertexShaderStage(resources.getVertexShader("LitMirroredModel"))
                                                                  .addFragmentShaderStage(resources.getFragmentShader("LitModel"))
                                                                  .addVulkVertexInput(0)
                                                                  .setCullMode(VK_CULL_MODE_FRONT_BIT)
                                                                  .build(mirrorWorldRenderStateDS->descriptorSetLayout);

        // pass ownership of everything off to the renderables.
        wallRenderable = std::make_unique<MeshRenderable>(vk, resources.getMesh("wall").mesh, std::move(wallDS), std::move(wallPipeline));
        skullRenderable = std::make_unique<MeshRenderable>(vk, resources.getMesh("skull").mesh, std::move(skullDS), std::move(skullPipeline));
        mirrorRenderable = std::make_unique<MeshRenderable>(vk, resources.getMesh("mirror").mesh, std::move(mirrorDS), std::move(mirrorPipeline));

        mirrorStencilRenderState = std::make_unique<RenderState>(vk, std::move(mirrorStencilDS), std::move(mirrorStencilPipeline));
        mirrorWorldRenderState = std::make_unique<RenderState>(vk, std::move(mirrorWorldRenderStateDS), std::move(mirroredWorldPipeline));
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
        updateXformsUBO(*modelUBOs.xforms[currentFrame], viewport);
        *modelUBOs.eyePos[currentFrame] = camera.eye;

        skullRenderable->render(commandBuffer, currentFrame, viewport, scissor);
        // wallRenderable->render(commandBuffer, currentFrame, viewport, scissor);

        // first render the mirror stencil so we know which pixels to keep
        mirrorStencilRenderState->setRenderState(commandBuffer, currentFrame, viewport, scissor);

        // now render the mirrored world
        mirrorWorldRenderState->setRenderState(commandBuffer, currentFrame, viewport, scissor);
        skullRenderable->bindAndDrawMesh(commandBuffer);

        // finally render the mirror
        // mirrorRenderable->render(commandBuffer, currentFrame, viewport, scissor);
    }

    ~MirrorWorld() {}

private:
    void loadResources()
    {
        VulkMesh wallMesh;
        makeQuad(6.0f, 6.0f, 1, wallMesh);
        wallMesh.xform(glm::rotate(glm::mat4(1.0f), glm::radians(-45.0f), glm::vec3(1.0f, 0.0f, 0.f)));
        wallMesh.xform(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -.5f, 0.0f)));

        VulkMesh mirrorMesh;
        makeQuad(2.0f, 1.0f, 1, mirrorMesh);
        mirrorMesh.xform(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, .5f, 0.0f)));
        mirrorMesh.xform(glm::rotate(glm::mat4(1.0f), glm::radians(-45.0f), glm::vec3(1.0f, 0.0f, 0.f)));
        mirrorMesh.xform(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -.49f, 0.0f)));

        auto wallResources = std::make_unique<MeshResources>(vk, std::move(wallMesh), "RedBrick/RedBrick.jpg", "RedBrick/RedBrickNormal.jpg");
        auto skullResources = std::make_unique<MeshResources>(vk, VulkMesh::loadFromFile("Assets/Models/Skull/Skull.obj"), "Skull/DiffuseMap.png", "Skull/NormalMap.png");
        auto mirrorResources = std::make_unique<MeshResources>(vk, std::move(mirrorMesh), "Brass-4K/4K-Brass_Base Color.jpg", "Brass-4K/4K-Brass_Normal.jpg");

        resources.addMesh("wall", std::move(wallResources))
            .addMesh("skull", std::move(skullResources))
            .addMesh("mirror", std::move(mirrorResources));
    }
};
