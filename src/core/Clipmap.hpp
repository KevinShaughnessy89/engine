#pragma once

// GL format traits per element type -- add a specialization here to support a new Clipmap<T>.
template <typename T>
struct ClipmapFormat;

template <>
struct ClipmapFormat<float> {
    static constexpr GLenum internalFormat = GL_R32F;
    static constexpr GLenum format = GL_RED;
};

template <>
struct ClipmapFormat<glm::vec2> {
    static constexpr GLenum internalFormat = GL_RG32F;
    static constexpr GLenum format = GL_RG;
};

template <typename T>
struct Clipmap {
   public:
    GLuint buffer = 0;
    int bufferSize = 0;
    float texelSize = 0.0f;  // world units per texel

    // No GL calls here -- Clipmap is often a member of something constructed before a GL context
    // exists (e.g. ChunkModel, which lives inside ChunkManager, which is built by a namespace-scope
    // global initializer that runs before main()/glfwInit). Call init() once a context is current.
    Clipmap() = default;
    ~Clipmap();

    void init(int newBufferSize, float newTexelSize);
    void upload(int x, int z, int blockWidth, int blockHeight, const std::vector<T>& data);
    int wrap(int v, int n) { return ((v % n) + n) % n; }
    void uploadRectangle(int destX, int destZ, int srcColOffset, int srcRowOffset, int width,
                         int height, int srcRowStride, const std::vector<T>& data);

    GLuint getTextureID() const { return buffer; }
};

extern template struct Clipmap<float>;
extern template struct Clipmap<glm::vec2>;