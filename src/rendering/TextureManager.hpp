#pragma once
#include <filesystem>
#include <mutex>

#include "UniformData.hpp"
#include "core/Core.hpp"
#include "core/Module.hpp"
#include "core/PrecompiledHeader.hpp"
#include "rendering/Texture.hpp"

struct TextureFile {
    std::string filename;       // e.g. "texture.png"
    std::string fullPath;       // full path like "/assets/textures/texture.png"
    std::string cacheFilename;  // full path + ".cache"
    std::string nameNoExt;      // filename without extension, e.g. "texture"
    std::string ext;

    TextureFile(const std::filesystem::directory_entry& entry) {
        filename = entry.path().filename().string();
        fullPath = entry.path().string();
        cacheFilename = (entry.path().parent_path() / entry.path().stem()).string() + ".cache";
        nameNoExt = entry.path().stem().string();
        ext = entry.path().extension().string();
    }
};

class TextureManager : public Module {
   public:
    // WARNING - texture storage is not consistently using TextureManager. Only the models.json
    // objects involve TextureManager
    TextureManager() : Module() {}
    void doInitialization() override;
    void addTextureAsync(std::future<void>&& textureProcess);
    void addDirectory(std::string directory) { directories.push_back(directory); }
    bool hasValidTextureExtension(const std::filesystem::path& path);
    void createTextureFiles();
    void loadTextureDataFromFiles();
    void loadSingleTextureData(const std::string& texturePath, const std::string& name,
                               std::string& filename);
    void loadTextures();
    void saveTextureData(TextureData& data, const std::string& name, const std::string fullPath);
    void removeTexture(const std::string& name);
    void loadTextureData(const std::string& filename, const std::string name);
    Texture* getTexture(const std::string& name);
    Texture* getTextureAs(const std::string currentName, const std::string newName);
    void setUniform(const std::string& name, std::string uniform);
    // Ownership transfers here; callers keep using the raw pointer returned by getTexture().
    void addTexture(const std::string& name, std::unique_ptr<Texture> texture) {
        textures[name] = std::move(texture);
    }
    std::vector<TextureUniformData> globalTextureUniforms = {};

   private:
    std::vector<std::future<void>> loadingTextures;
    std::vector<std::string> directories;
    std::unordered_map<std::string, std::unique_ptr<Texture>> textures;
    std::unordered_map<std::string, std::unique_ptr<TextureData>> textureData;
    std::vector<TextureFile> textureFiles;
    std::mutex textureDataMutex;
    std::mutex texturesMutex;
    std::atomic<int> texturesRemaining;
};
