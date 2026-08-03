#pragma once
#include <iomanip>
#include <iostream>
#include <memory>

#include "core/PrecompiledHeader.hpp"
#include "tinyexr-release/tinyexr.h"

class ShaderProgram;

// stbi_load allocates with malloc (freed via stbi_image_free); EXR decoding and the cache-file
// path both allocate with new[] (freed via delete[]) -- the deleter has to know which one
// produced a given buffer, so the two can't be freed interchangeably.
enum class TextureDataOrigin { None, Stbi, HeapArray };

struct TextureDataDeleter {
    TextureDataOrigin origin = TextureDataOrigin::None;
    void operator()(unsigned char* data) const {
        if (!data) return;
        if (origin == TextureDataOrigin::Stbi) {
            stbi_image_free(data);
        } else {
            delete[] data;
        }
    }
};
using TextureBuffer = std::unique_ptr<unsigned char[], TextureDataDeleter>;

struct TextureData {
    std::string name;
    std::string filename;
    int width = 0;
    int height = 0;
    int nrChannels = 0;
    TextureBuffer data;
    TextureData() = default;

    TextureData(const std::string& texturePath, const std::string& name, std::string& filename)
        : name(name), filename(filename) {
        getTextureData(texturePath);
    }

    TextureData(const std::string& name, const std::string& filename, int width, int height,
                int nrChannels, unsigned char* data, TextureDataOrigin origin)
        : name(name),
          filename(filename),
          width(width),
          height(height),
          nrChannels(nrChannels),
          data(data, TextureDataDeleter{origin}) {}

    bool getTextureData(const std::string& texturePath) {
        bool isExr = texturePath.substr(texturePath.find_last_of(".") + 1) == "exr";

        if (isExr) {
            float* out;
            int width, height;
            const char* err = nullptr;

            int ret = LoadEXR(&out, &width, &height, texturePath.c_str(), &err);
            if (ret != TINYEXR_SUCCESS) {
                std::cerr << "Failed to load EXR texture: " << err << std::endl;
                return false;
            }

            this->width = width;
            this->height = height;
            this->nrChannels = 4;  // EXR is typically RGBA

            int count = width * height * nrChannels;
            unsigned char* temp = new unsigned char[count];
            for (int i = 0; i < count; ++i) {
                float v = out[i];
                v = v * 0.5f + 0.5f;  // Normalize to [0, 1]
                v = std::clamp(v, 0.0f, 1.0f);
                temp[i] = static_cast<unsigned char>(v * 255.0f + 0.5f);
            }
            free(out);  // tinyexr allocates this with malloc; was never freed before
            data = TextureBuffer(temp, TextureDataDeleter{TextureDataOrigin::HeapArray});
            return true;
        } else {
            unsigned char* loaded = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 0);
            if (!loaded) {
                std::cout << "Texture data failed to load at path: " << texturePath << std::endl;
                return false;
            }
            data = TextureBuffer(loaded, TextureDataDeleter{TextureDataOrigin::Stbi});
            return true;
        }
    }

    void printSummary(int maxPixels = 10) const {
        std::cout << "TextureData Summary: \n";
        std::cout << "  Name: " << name << "\n";
        std::cout << "  Dimensions: " << width << "x" << height << "\n";
        std::cout << "  Channels: " << nrChannels << "\n";

        if (!data) {
            std::cout << "  Data pointer is null.\n";
            return;
        }

        int totalPixels = width * height;
        int pixelsToPrint = std::min(maxPixels, totalPixels);

        std::cout << "  First " << pixelsToPrint << " pixels (hex):\n    ";

        // Each pixel has nrChannels bytes (e.g. 3 for RGB, 4 for RGBA)
        for (int i = 0; i < pixelsToPrint; ++i) {
            std::cout << "[";
            for (int c = 0; c < nrChannels; ++c) {
                // Print each channel as 2-digit hex
                unsigned char val = data[i * nrChannels + c];
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)val;
                if (c < nrChannels - 1) std::cout << " ";
            }
            std::cout << "] ";
        }
        std::cout << std::dec << "\n";  // reset number base to decimal
    }
};

class Texture {
   public:
    Texture() {}
    Texture(const std::string& name, const std::string& uniform = "");
    Texture(GLuint texture, const std::string& name, const std::string& uniform = "")
        : name(name), uniform(uniform), textureID(texture) {}
    virtual ~Texture() = default;
    GLuint textureID;
    std::string name;
    std::string uniform;
    virtual void loadTexture(const std::vector<TextureData*>& textureData) = 0;
    virtual void bindTexture(ShaderProgram* shader, unsigned int textureUnit = 0) = 0;
    virtual void generateFallbackTexture() = 0;
    virtual void setUniform(std::string uniform);
};

class Texture2D : public Texture {
   public:
    Texture2D(const std::vector<TextureData*>& textureData, const std::string& name,
              const std::string& uniform = "");
    Texture2D(GLuint texture, const std::string& name, const std::string& uniform = "")
        : Texture(texture, name, uniform) {
        glObjectLabel(GL_TEXTURE, textureID, -1, name.c_str());
    }
    Texture2D() : Texture() {}
    void loadTexture(const std::vector<TextureData*>& textureData) override;
    void bindTexture(ShaderProgram* shader, unsigned int textureUnit = 0) override;
    void generateFallbackTexture() override;
};

class TextureCube : public Texture {
   public:
    TextureCube(const std::vector<TextureData*>& textureData, const std::string& name,
                const std::string& uniform = "");
    TextureCube(GLuint texture, const std::string& name, const std::string& uniform = "");
    void loadTexture(const std::vector<TextureData*>& textureData) override;
    void bindTexture(ShaderProgram* shader, unsigned int textureUnit = 0) override;
    void generateFallbackTexture() override;
};

class TextureDataMap : public Texture {
   public:
    TextureDataMap(const std::vector<TextureData*>& textureData, const std::string& name,
                   const std::string& uniform = "");
    void loadTexture(const std::vector<TextureData*>& textureData) override;
    void bindTexture(ShaderProgram* shader, unsigned int textureUnit = 0) override;
    void generateFallbackTexture() override {}
};

class TextureDataArray2D : public Texture {
   public:
    int width, height, layerCount, nextFreeLayer;
    TextureDataArray2D(const std::string& name, int layerCount, const std::string& uniform = "");
    virtual void loadTexture(const std::vector<TextureData*>& textureData) override {}
    void loadTexture(const std::string& texturePath);
    void loadTexture(std::vector<GLuint> textureIDs);
    void bindTexture(ShaderProgram* shader, unsigned int textureUnit = 0) override;
    void generateFallbackTexture() override {}
};

class TextureArray2D : public Texture {
   public:
    int width, height, layerCount, nextFreeLayer;
    TextureArray2D(const std::string& name, int layerCount, const std::string& uniform = "");
    virtual void loadTexture(const std::vector<TextureData*>& textureData) override {}
    void loadTexture(const std::string& texturePath);
    void loadTexture(std::vector<GLuint> textureIDs);
    void bindTexture(ShaderProgram* shader, unsigned int textureUnit = 0) override;
    void generateFallbackTexture() override {}
};
