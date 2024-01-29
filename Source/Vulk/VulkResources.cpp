#include "VulkResources.h"

#include <unordered_map>
#include <iostream>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

void processModelDir(const fs::path &dirPath)
{

    // Your custom processing logic for the directory
    std::cout << "Processing directory: " << dirPath << std::endl;
    // ... add your processing code here ...
}

void findAndProcessModels(const fs::path &path)
{
    assert(fs::exists(path) && fs::is_directory(path));
    for (const auto &entry : fs::recursive_directory_iterator(path))
    {
        if (entry.is_regular_file() && entry.path().filename() == "model.json")
        {
            processModelDir(entry.path().parent_path());
        }
    }
}

void VulkResources::loadResources(std::string build_dir)
{
    std::string path = build_dir + "/Assets";
    findAndProcessModels(path + "/Models");
}