#include "core/PrecompiledHeader.hpp"
#define ENABLE_LOGGING

#include <cassert>

#include "Texture.hpp"
#include "TextureManager.hpp"
#include "common/log.hpp"
#include "rendering/AssetFilePath.hpp"
#include "rendering/ResourceManager.hpp"

void TextureManager::doInitialization() {
    addDirectory(Config::ProjectRootDir + "/" + AssetFilePath::terrainTextures);
    addDirectory(Config::ProjectRootDir + "/" + AssetFilePath::skyboxTextures);
    addDirectory(Config::ProjectRootDir + "/" + AssetFilePath::objectTextures);
    addDirectory(Config::ProjectRootDir + "/" + AssetFilePath::particleTextures);
    addDirectory(Config::ProjectRootDir + "/" + AssetFilePath::assetsRoot);

    createTextureFiles();
    loadTextureDataFromFiles();

    for (auto& future : loadingTextures) {
        future.wait();
    }
}

void TextureManager::loadSingleTextureData(const std::string& texturePath, const std::string& name,
                                           std::string& filename) {
    TextureData* newTextureData = new TextureData(texturePath, name, filename);
    saveTextureData(*newTextureData, name, texturePath);
    {
        std::lock_guard<std::mutex> lock(textureDataMutex);
        textureData[name] = newTextureData;
    }
}

bool TextureManager::hasValidTextureExtension(const std::filesystem::path& path) {
    static const std::vector<std::string> validExtensions = {".jpg", ".jpeg", ".png",
                                                             ".bmp", ".tga",  ".exr"};
    auto ext = path.extension().string();

    // convert extension to lowercase for case-insensitive compare
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return std::find(validExtensions.begin(), validExtensions.end(), ext) != validExtensions.end();
}

void TextureManager::createTextureFiles() {
    for (const auto& directory : directories) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file() && hasValidTextureExtension(entry)) {
                auto it = std::find_if(textureFiles.begin(), textureFiles.end(),
                                       [&entry](const TextureFile& file) {
                                           return file.nameNoExt == entry.path().stem().string();
                                       });

                if (it == textureFiles.end()) {
                    textureFiles.emplace_back(entry);
                } else {
                    if (entry.path().extension().string() == ".cache") {
                        textureFiles.erase(it);
                        textureFiles.emplace_back(entry);
                    }
                    if (it->ext == ".cache") {
                        // Let the iteration return
                    }
                }
            }
        }
    }
    for (auto texture : textureFiles) {
    }
    texturesRemaining = textureFiles.size();
}

void TextureManager::loadTextureDataFromFiles() {
    for (const auto& file : textureFiles) {
        std::string nameNoExt = file.nameNoExt;
        std::string cacheFilename = file.cacheFilename;
        std::string fullPath = file.fullPath;
        std::string filename = file.filename;

        // Check if texture is already loaded
        if (textureData.find(nameNoExt) != textureData.end()) {
            continue;
        }

        std::ifstream cacheFile(cacheFilename, std::ios::binary);
        if (cacheFile.is_open()) {
            // Load from cache on separate thread
            addTextureAsync(
                std::async(std::launch::async, [this, cacheFilename, nameNoExt]() mutable {
                    loadTextureData(cacheFilename, nameNoExt);
                }));
        } else {
            // Load texture data on separate thread
            addTextureAsync(
                std::async(std::launch::async, [this, fullPath, nameNoExt, filename]() mutable {
                    loadSingleTextureData(fullPath, nameNoExt, filename);
                }));
        }
    }
}

void TextureManager::loadTextures() {
    std::vector<TextureData*> skyboxTextures;
    std::vector<TextureData*> texture2D;
    for (const auto& [name, data] : textureData) {
        if (name.substr(0, 7) == "skybox_") {
            skyboxTextures.push_back(data);
        } else if (name.substr(0, 5) == "data_") {
            texture2D.push_back(data);
            textures.emplace(name, new TextureDataMap(texture2D, name));
            texture2D.clear();
        } else if (name.substr(0, 5) == "cube_") {
            skyboxTextures.push_back(data);

        } else {
            texture2D.push_back(data);
            textures.emplace(name, new Texture2D(texture2D, name));
            texture2D.clear();
        }
    }
    textures.emplace("skyboxTexture", new TextureCube(skyboxTextures, "skyboxTexture"));
}

void TextureManager::removeTexture(const std::string& name) {
    auto it = textures.find(name);
    if (it != textures.end()) {
        delete it->second;   // Free the memory of the Texture object
        textures.erase(it);  // Remove the entry from the map
    } else {
        std::cout << "Warning: texture " << name << " not found for removal." << std::endl;
    }
}

void TextureManager::saveTextureData(TextureData& data, const std::string& name,
                                     const std::string fullPath) {
    std::string filename = name + ".cache";

    size_t lastSlash = fullPath.rfind("/");
    std::string dir = fullPath.substr(0, lastSlash) + "/";
    std::cout << "Data texture saved for " << name << std::endl;

    std::string outputPath = dir + filename;

    std::ofstream file(outputPath, std::ios::binary);

    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(&data.width), sizeof(int));
        file.write(reinterpret_cast<const char*>(&data.height), sizeof(int));
        file.write(reinterpret_cast<const char*>(&data.nrChannels), sizeof(int));
        file.write(reinterpret_cast<const char*>(data.data),
                   data.width * data.height * data.nrChannels);
    } else {
        std::cerr << "Failed to write to file " << outputPath << std::endl;
    }
}

void TextureManager::loadTextureData(const std::string& filename, const std::string name) {
    TextureData* data = new TextureData();
    data->name = name;
    data->filename = filename;
    data->wasCached = true;
    std::ifstream file(filename, std::ios::binary);
    file.read(reinterpret_cast<char*>(&data->width), sizeof(int));
    file.read(reinterpret_cast<char*>(&data->height), sizeof(int));
    file.read(reinterpret_cast<char*>(&data->nrChannels), sizeof(int));

    data->data = new unsigned char[data->width * data->height * data->nrChannels];

    file.read(reinterpret_cast<char*>(data->data), data->height * data->width * data->nrChannels);
    {
        std::lock_guard<std::mutex> lock(textureDataMutex);
        textureData[name] = data;
    }
}

Texture* TextureManager::getTextureAs(const std::string currentName, const std::string newName) {
    Texture* tex = getTexture(currentName);
    tex->name = newName;

    return tex;
}

Texture* TextureManager::getTexture(const std::string& name) {
    // strip any baked-in relative path, keep just the filename
    std::string textureName = std::string(name);
    size_t slash = textureName.find_last_of("/\\");
    if (slash != std::string::npos) {
        textureName = textureName.substr(slash + 1);
    }

    // trim extension if necessary
    size_t dot = textureName.find_last_of(".");
    if (dot != std::string::npos) {
        textureName = textureName.substr(0, dot);
    }
    auto it = textures.find(textureName);
    if (it != textures.end()) {
        if (it->second) {
            return it->second;
        }
        std::cout << "Warning: texture pointer is null for: " << textureName << std::endl;
        return nullptr;
    } else {
        std::cout << "Warning: texture " << textureName << " not found." << std::endl;
    }
    return nullptr;
}

void TextureManager::setUniform(const std::string& name, std::string uniform) {
    Texture* texture = getTexture(name);

    texture->setUniform(uniform);

    std::cout << "Set uniform for texture: " << name << " to " << uniform << std::endl;
}

void TextureManager::addTextureAsync(std::future<void>&& textureProcess) {
    loadingTextures.push_back(std::move(textureProcess));
}
