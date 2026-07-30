#pragma once

class TextureCube;

class ImageBasedLighting {
   public:
    ImageBasedLighting() = default;
    ~ImageBasedLighting() = default;

    GLuint environmentCubeMap, captureFBO, captureRBO, cubeVAO, cubeVBO, irradianceMap,
        irradianceFBO, irradianceRBO;

    void init();
    void generateEnvironmentMap();
    void generateIrradianceMap();
    void render();
    void renderCube();
};