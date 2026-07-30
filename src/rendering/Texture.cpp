#include "Texture.hpp"

#include <tinyexr.h>

#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"
#include "rendering/ShaderProgram.hpp"
#include "rendering/TextureManager.hpp"

Texture::Texture(const std::string& name, const std::string& uniform)
    : name(name), uniform(uniform) {
}

Texture2D::Texture2D(const std::vector<TextureData*>& textureData, const std::string& name,
                     const std::string& uniform)
    : Texture(name, uniform) {
    loadTexture(textureData);
}

TextureCube::TextureCube(const std::vector<TextureData*>& textureData, const std::string& name,
                         const std::string& uniform)
    : Texture(name, uniform) {
    loadTexture(textureData);
}

TextureCube::TextureCube(GLuint texture, const std::string& name, const std::string& uniform)
    : Texture(texture, name, uniform) {
    glObjectLabel(GL_TEXTURE, textureID, -1, name.c_str());
}

TextureDataMap::TextureDataMap(const std::vector<TextureData*>& textureData,
                               const std::string& name, const std::string& uniform)
    : Texture(name, uniform) {
    loadTexture(textureData);
}

void TextureDataMap::loadTexture(const std::vector<TextureData*>& textureData) {
    assert(textureData.size() == 1);

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glObjectLabel(GL_TEXTURE, textureID, -1, name.c_str());

    unsigned char* data = textureData[0]->data;

    if (!data) {
        std::cerr << "Failed to load normal map texture" << std::endl;
        return;
    }

    int width = textureData[0]->width;
    int height = textureData[0]->height;
    int channels = textureData[0]->nrChannels;

    GLenum format = 0;
    GLenum internalFormat;  // Force linear format

    switch (channels) {
        case 1:
            format = GL_RED;
            internalFormat = GL_R8;
            break;
        case 2:
            format = GL_RG;
            internalFormat = GL_RG8;
            break;
        case 3:
            format = GL_RGB;
            internalFormat = GL_RGBA8;
            break;
        case 4:
            format = GL_RGBA;
            internalFormat = GL_RGBA8;
            break;
        default:
            std::cerr << "Unsupported normal map format: " << channels << " channels\n";
            return;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE,
                 data);
    glGenerateMipmap(GL_TEXTURE_2D);

    if (textureData[0]->wasCached) {
        delete[] data;
    } else {
        if (textureData[0]->name != "fallback") {
            stbi_image_free(data);
        }
    }
    delete textureData[0];

    // Texture wrapping: normal maps should tile cleanly
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Filtering: enable trilinear filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Optional: Enable maximum anisotropic filtering for better angle quality
    GLfloat anisoSystem = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisoSystem);
    GLfloat aniso = std::min(anisoSystem, 4.0f);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
}

void TextureDataMap::bindTexture(ShaderProgram* shader, unsigned int textureUnit) {
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, textureID);

    assert(!uniform.empty() && "Texture uniform must be set before binding");
    shader->setInt(uniform, textureUnit);
}

void Texture2D::loadTexture(const std::vector<TextureData*>& textureData) {
    assert(textureData.size() == 1);

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glObjectLabel(GL_TEXTURE, textureID, -1, name.c_str());

    // Load image
    unsigned char* data = textureData[0]->data;
    if (data) {
        GLenum format = 0;
        GLenum internalFormat = 0;
        switch (textureData[0]->nrChannels) {
            case 1:
                format = GL_RED;
                internalFormat = GL_R8;
                break;
            case 3:
                format = GL_RGB;
                internalFormat = GL_SRGB8;
                break;
            case 4:
                format = GL_RGBA;
                internalFormat = GL_SRGB8_ALPHA8;
                break;
            default:
                std::cerr << "Unsupported nrChannels: " << textureData[0]->nrChannels << std::endl;
                return;
        }
        // GL_UNPACK_ALIGNMENT defaults to 4, which makes the driver assume each row is padded to
        // a 4-byte boundary. stbi's buffers are tightly packed, so any width*channels not a
        // multiple of 4 (common for non-power-of-two embedded textures) makes it read past the
        // buffer on the last row.
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, textureData[0]->width,
                     textureData[0]->height, 0, format, GL_UNSIGNED_BYTE, textureData[0]->data);
        glGenerateMipmap(GL_TEXTURE_2D);

        if (textureData[0]->wasCached) {
            delete[] data;
        } else {
            if (textureData[0]->name != "fallback") {
                stbi_image_free(data);
            }
        }

    } else {
        std::cout << "Failed to load texture" << std::endl;
    }
    // glBindVertexArray(0);

    // Set texture wrapping and filtering options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Without this, ground/wall textures viewed at a shallow angle get forced to a much blurrier
    // mip level than needed, since the GPU otherwise picks mips off the worst-case screen-space
    // derivative across the surface.
    GLfloat aniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
}

void Texture2D::generateFallbackTexture() {
    int size = 64;
    int checkerSize = 8;

    unsigned char* data = new unsigned char[size * size * 3];  // RGB format

    for (int x = 0; x < size; x++) {
        for (int y = 0; y < size; y++) {
            unsigned char color = ((x / checkerSize) + (y / checkerSize)) % 2 ? 255 : 30;

            int i = (y * size + x) * 3;
            data[i] = color;
            data[i + 1] = color;
            data[i + 2] = color;
        }
    }

    TextureData* textureData = new TextureData("fallback", "null", size, size, 3, data, false);
    this->name = "fallback";
    this->uniform = "";

    loadTexture(std::vector<TextureData*>({textureData}));
}

void TextureCube::loadTexture(const std::vector<TextureData*>& textureData) {
    assert(textureData.size() == 6);
    GLenum faceText[6] = {GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
                          GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
                          GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z};

    std::vector<std::string> faces = {"skybox_right",  "skybox_left",  "skybox_top",
                                      "skybox_bottom", "skybox_front", "skybox_back"};

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    glObjectLabel(GL_TEXTURE, textureID, -1, name.c_str());

    int i = 0;
    for (const auto& face : faces) {
        auto it = std::find_if(textureData.begin(), textureData.end(),
                               [&](const TextureData* data) { return data->name == face; });

        if (it != textureData.end()) {
            GLenum internalFormat = ((*it)->nrChannels == 3) ? GL_RGB8 : GL_RGBA8;
            GLenum format = ((*it)->nrChannels == 3) ? GL_RGB : GL_RGBA;
            glTexImage2D(faceText[i], 0, internalFormat, (*it)->width, (*it)->height, 0, format,
                         GL_UNSIGNED_BYTE, (*it)->data);

            if ((*it)->wasCached) {
                delete[] (*it)->data;
            } else {
                stbi_image_free((*it)->data);
            }

            i++;
        } else {
            std::cout
                << "Cubemap files missing or inconsistently named. Loading fallback texture..."
                << std::endl;
            generateFallbackTexture();
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void TextureCube::generateFallbackTexture() {
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    glObjectLabel(GL_TEXTURE, textureID, -1, name.c_str());

    unsigned char color[3] = {20, 20, 20};
    int size = 1;

    for (int face = 0; face < 6; face++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB8, 64, 64, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, color);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void Texture2D::bindTexture(ShaderProgram* shader, unsigned int textureUnit) {
    // Activate the desired texture unit
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    assert(!uniform.empty() && "Texture uniform must be set before binding");
    // Set the texture unit in the shader
    shader->setInt(uniform, textureUnit);
    // Bind the texture
    glBindTexture(GL_TEXTURE_2D, textureID);
}

void TextureCube::bindTexture(ShaderProgram* shader, unsigned int textureUnit) {
    // Activate the desired texture unit
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    assert(!uniform.empty() && "Texture uniform must be set before binding");
    // Set the texture unit in the shader
    shader->setInt(uniform, textureUnit);

    // Bind the texture
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
}

void Texture::setUniform(std::string uniform) {
    this->uniform = uniform;
}

TextureDataArray2D::TextureDataArray2D(const std::string& name, int layerCount,
                                       const std::string& uniform)
    : Texture(name, uniform), width(0), height(0), layerCount(layerCount), nextFreeLayer(0) {
}

void TextureDataArray2D::loadTexture(const std::string& texturePath) {
    const std::string stupidCode = "";
    TextureData* newTextureData = new TextureData(
        texturePath, stupidCode, name);  // should remove the filename arg for TextureData, rethink
                                         // name concept I've never used it

    if (nextFreeLayer == 0) {
        width = newTextureData->width;
        height = newTextureData->height;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);
        glObjectLabel(GL_TEXTURE, textureID, -1, name.c_str());
        glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, width, height, layerCount);
    } else {
        glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);
    }

    // Every layer shares the storage allocated for layer 0 -- uploading a layer of a different
    // size would tell glTexSubImage3D to read width*height*channels bytes from a buffer that was
    // only decoded to the mismatched image's (possibly smaller) actual size, reading past the end
    // of newTextureData->data and crashing with an access violation.
    if (newTextureData->width != width || newTextureData->height != height) {
        std::cerr << "Texture array layer \"" << texturePath << "\" is " << newTextureData->width
                  << "x" << newTextureData->height << ", but the array was created at " << width
                  << "x" << height << " from its first layer -- skipping upload.\n";
    } else {
        GLenum format = newTextureData->nrChannels == 4 ? GL_RGBA : GL_RGB;
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                        0,                    // mip level -- storage above only has level 0
                        0, 0, nextFreeLayer,  // xoffset, yoffset, zoffset (layer)
                        width, height, 1,     // width, height, depth=1 layer
                        format, GL_UNSIGNED_BYTE, newTextureData->data);
    }
    nextFreeLayer++;

    if (newTextureData->wasCached) {
        delete[] newTextureData->data;
    } else {
        stbi_image_free(newTextureData->data);
    }
    delete newTextureData;
}

void TextureDataArray2D::bindTexture(ShaderProgram* shader, unsigned int textureUnit) {
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);
    assert(!uniform.empty() && "Texture uniform must be set before binding");
    // Set the texture unit in the shader
    shader->setInt(uniform, textureUnit);
}

TextureArray2D::TextureArray2D(const std::string& name, int layerCount, const std::string& uniform)
    : Texture(name, uniform), width(0), height(0), layerCount(layerCount), nextFreeLayer(0) {
}

void TextureArray2D::loadTexture(const std::string& texturePath) {
    const std::string stupidCode = "";
    TextureData* newTextureData = new TextureData(
        texturePath, stupidCode, name);  // should remove the filename arg for TextureData, rethink
                                         // name concept I've never used it

    if (nextFreeLayer == 0) {
        width = newTextureData->width;
        height = newTextureData->height;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);
        glObjectLabel(GL_TEXTURE, textureID, -1, name.c_str());
        // sRGB8_ALPHA8 belongs in the storage's internalformat, not the TexSubImage3D format arg
        // -- format there must be an unsized enum describing the source data's layout.
        // Full mip chain (not just level 0) -- terrain.frag's near/far distance blend divides by
        // (textureQueryLevels() - 1), which was always 0 with a single-level allocation, so that
        // blendFactor was NaN/inf per-fragment depending on textureQueryLod's sign.
        int mipLevels = static_cast<int>(std::floor(std::log2(std::max(width, height)))) + 1;
        glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevels, GL_SRGB8_ALPHA8, width, height, layerCount);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);
    }

    // Every layer shares the storage allocated for layer 0 -- uploading a layer of a different
    // size would tell glTexSubImage3D to read width*height*channels bytes from a buffer that was
    // only decoded to the mismatched image's (possibly smaller) actual size, reading past the end
    // of newTextureData->data and crashing with an access violation.
    if (newTextureData->width != width || newTextureData->height != height) {
        std::cerr << "Texture array layer \"" << texturePath << "\" is " << newTextureData->width
                  << "x" << newTextureData->height << ", but the array was created at " << width
                  << "x" << height << " from its first layer -- skipping upload.\n";
    } else {
        GLenum format = newTextureData->nrChannels == 4 ? GL_RGBA : GL_RGB;
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                        0,                    // mip level -- storage above only has level 0
                        0, 0, nextFreeLayer,  // xoffset, yoffset, zoffset (layer)
                        width, height, 1,     // width, height, depth=1 layer
                        format, GL_UNSIGNED_BYTE, newTextureData->data);
    }
    nextFreeLayer++;

    // Regenerate after every layer rather than waiting for nextFreeLayer == layerCount -- that
    // gate silently never fired (and left every mip above 0 as black, zero-initialized storage)
    // whenever fewer layers get loaded than the array was constructed for, e.g. while testing with
    // a subset of layers commented out in ChunkModel.cpp. This is load-time only, not a per-frame
    // cost, so regenerating a few extra times is free correctness insurance.
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

    if (newTextureData->wasCached) {
        delete[] newTextureData->data;
    } else {
        stbi_image_free(newTextureData->data);
    }
    delete newTextureData;
}

void TextureArray2D::bindTexture(ShaderProgram* shader, unsigned int textureUnit) {
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);
    assert(!uniform.empty() && "Texture uniform must be set before binding");
    // Set the texture unit in the shader
    shader->setInt(uniform, textureUnit);
}