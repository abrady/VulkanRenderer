// only to be included by VulkResources.cpp
#pragma once

#include <unordered_map>
#include <iostream>
#include <filesystem>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#define ACTOR_JSON_VERSION 1
struct ActorDef
{
    uint32_t version;
    std::string name;
    std::string pipeline;
    std::string mesh;

    // not parsed from JSON
    std::filesystem::path directoryPath;

    void validate()
    {
        assert(version == ACTOR_JSON_VERSION);
        assert(!name.empty());
        assert(!pipeline.empty());
        assert(!mesh.empty());
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ActorDef, version, name, pipeline, mesh);
};

#define MODEL_JSON_VERSION 1

// Serialize VulkMaterial to JSON
void to_json(json &j, const VulkMaterial &material)
{
    j = json{
        {"diffuse", {material.diffuse.r, material.diffuse.g, material.diffuse.b, material.diffuse.a}},
        {"fresnelR0", {material.fresnelR0.r, material.fresnelR0.g, material.fresnelR0.b}},
        {"roughness", material.roughness}};
}

// Deserialize JSON to VulkMaterial
void from_json(const nlohmann::json &j, VulkMaterial &material)
{
    std::vector<float> diffuse = j.at("diffuse").get<std::vector<float>>();
    material.diffuse = glm::vec4(diffuse[0], diffuse[1], diffuse[2], diffuse[3]);
    std::vector<float> fresnelR0 = j.at("fresnelR0").get<std::vector<float>>();
    material.fresnelR0 = glm::vec3(fresnelR0[0], fresnelR0[1], fresnelR0[2]);
    material.roughness = j.at("roughness").get<float>();
}

struct ModelDef
{
    uint32_t version;
    std::string name;
    std::string mesh;
    std::string texture;
    std::string normalMap;
    VulkMaterial material;

    // not parsed from JSON
    std::filesystem::path directoryPath;

    void validate()
    {
        assert(version == MODEL_JSON_VERSION);
        assert(!name.empty());
        assert(!mesh.empty());
        assert(!texture.empty());
        assert(!normalMap.empty());
    }

    // Serialize VulkMaterial to JSON
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ModelDef, version, name, mesh, texture, normalMap, material);
};

struct Metadata
{
    std::unordered_map<std::string, ModelDef> models;
    std::unordered_map<std::string, ActorDef> actors;
} metadata;

template <typename T>
void processMetadataFile(const std::filesystem::path &file, std::unordered_map<std::string, T> &defs)
{
    assert(std::filesystem::exists(file) && std::filesystem::is_regular_file(file));
    std::cout << "processing " << file << std::endl;
    std::ifstream f(file);
    nlohmann::json j = nlohmann::json::parse(f);
    T m = j.template get<T>();
    m.validate();
    assert(defs.find(m.name) == defs.end());
    m.directoryPath = file.parent_path();
    defs[m.name] = m;
    std::cout << "added " << m.name << "from " << file << std::endl;
}

void findAndProcessMetadata(const std::filesystem::path &path)
{
    assert(std::filesystem::exists(path) && std::filesystem::is_directory(path));
    for (const auto &entry : std::filesystem::recursive_directory_iterator(path))
    {
        if (entry.is_regular_file())
        {
            if (entry.path().filename() == "model.json")
            {
                processMetadataFile(entry.path(), metadata.models);
            }
            else if (entry.path().filename() == "actor.json")
            {
                processMetadataFile(entry.path(), metadata.actors);
            }
        }
    }
}