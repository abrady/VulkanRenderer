#pragma once

#include <unordered_map>
#include <iostream>
#include <filesystem>
#include <string>

#include "Vulk.h"
#include "VulkTextureView.h"
#include "VulkMesh.h"

class VulkResources
{
public:
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

private:
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

    void loadResources(std::string build_dir);

public:
    VulkResources(Vulk &vk)
        : vk(vk)
    {
        textureSampler = vk.createTextureSampler();
    }

    VulkResources &addMesh(std::string const &name, std::unique_ptr<MeshResources> &&mesh)
    {
        ASSERT_KEY_NOT_SET(meshResources, name);
        meshResources[name] = std::move(mesh);
        return *this;
    }

    VulkResources &loadVertexShader(std::string name)
    {
        ASSERT_KEY_NOT_SET(vertShaders, name);
        VkShaderModule shaderModule = createShaderModule(Vertex, name);
        vertShaders[name] = shaderModule;
        return *this;
    }

    VulkResources &loadFragmentShader(std::string name)
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

    ~VulkResources()
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