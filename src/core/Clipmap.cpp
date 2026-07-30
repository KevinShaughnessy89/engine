#include "core/PrecompiledHeader.hpp"
#include "core/Clipmap.hpp"

template <typename T>
void Clipmap<T>::init(int newBufferSize, float newTexelSize) {
    bufferSize = newBufferSize;
    texelSize = newTexelSize;
    if (buffer != 0) glDeleteTextures(1, &buffer);
    glGenTextures(1, &buffer);
    glBindTexture(GL_TEXTURE_2D, buffer);
    glTexImage2D(GL_TEXTURE_2D, 0, ClipmapFormat<T>::internalFormat, bufferSize, bufferSize, 0,
                 ClipmapFormat<T>::format, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

template <typename T>
void Clipmap<T>::upload(int x, int z, int blockWidth, int blockHeight, const std::vector<T>& data) {
    int wrappedX = wrap(x, bufferSize);
    int wrappedZ = wrap(z, bufferSize);

    bool wrapsX = wrappedX + blockWidth > bufferSize;
    bool wrapsZ = wrappedZ + blockHeight > bufferSize;

    int xSplit = wrapsX ? (bufferSize - wrappedX) : blockWidth;
    int zSplit = wrapsZ ? (bufferSize - wrappedZ) : blockHeight;

    uploadRectangle(wrappedX, wrappedZ, 0, 0, xSplit, zSplit, blockWidth, data);

    // x maps to col, z maps to row
    if (wrapsX)
        uploadRectangle(0, wrappedZ, xSplit, 0, blockWidth - xSplit, zSplit, blockWidth, data);
    if (wrapsZ)
        uploadRectangle(wrappedX, 0, 0, zSplit, xSplit, blockHeight - zSplit, blockWidth, data);
    if (wrapsZ && wrapsX)
        uploadRectangle(0, 0, xSplit, zSplit, blockWidth - xSplit, blockHeight - zSplit,
                        blockWidth, data);
}

template <typename T>
void Clipmap<T>::uploadRectangle(int destX, int destZ, int srcColOffset, int srcRowOffset, int width,
                                 int height, int srcRowStride, const std::vector<T>& data) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Width and height must be positive");
    }

    const T* src = data.data() + srcRowOffset * srcRowStride + srcColOffset;

    glPixelStorei(GL_UNPACK_ROW_LENGTH, srcRowStride);
    glBindTexture(GL_TEXTURE_2D, buffer);
    glTexSubImage2D(GL_TEXTURE_2D, 0, destX, destZ, width, height, ClipmapFormat<T>::format,
                    GL_FLOAT, src);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);  // Let's not leak state plz
}

template <typename T>
Clipmap<T>::~Clipmap() {
    glDeleteTextures(1, &buffer);
}

template struct Clipmap<float>;
template struct Clipmap<glm::vec2>;
