#include "Biomes.hpp"

// struct Biome {
//     float targetX, targetZ;
//     float macroAmplitude;
//     float macroFrequency;
//     float detailAmplitude;
//     float microFrequency;
//     float persistence;
//     int octaves;
//     float lacunarity;
// };

std::vector<Biome> biomes = {
    // PLAINS - gentle, rolling terrain (quadrant: east)
    {0.3f, 0.0f, 20.0f, 0.003f, 0.5f, 0.001f, 10.0f, 3, 1.8f},
    {0.45f, 0.15f, 25.0f, 0.0025f, 1.5f, 0.001f, 10.0f, 3, 1.8f},
    {0.6f, 0.3f, 30.0f, 0.002f, 2.0f, 0.001f, 10.0f, 4, 1.8f},

    // MOUNTAINS - tall but broad: amplitude stays a modest fraction of wavelength (1/freq) so the
    // raw fBm base slopes ~45 deg worst-case instead of shearing into needles; erosion adds drama.
    {-0.45f, 0.45f, 8000.0f, 0.00003f, 1000.0f, 0.001f, 0.5f, 3, 2.0f, true},
    {-0.675f, 0.675f, 12000.0f, 0.000025f, 1000.0f, 0.001f, 0.5f, 3, 2.0f, true},
    {-0.9f, 0.9f, 16000.0f, 0.0000225f, 1000.0f, 0.001f, 0.5f, 3, 2.0f, true},

    // HILLS - moderate rolling terrain (quadrant: south)
    {0.0f, -0.3f, 30.0f, 0.0012f, 3.0f, 0.02f, 0.5f, 3, 1.9f},
    {0.15f, -0.45f, 60.0f, 0.001f, 3.5f, 0.018f, 0.5f, 4, 1.9f},
    {0.3f, -0.6f, 80.0f, 0.0009f, 4.0f, 0.015f, 0.5f, 4, 1.9f},

    // DESERT - smooth with occasional dunes/features (quadrant: west)
    {-0.3f, -0.3f, 15.0f, 0.0015f, 2.0f, 0.015f, 0.3f, 3, 1.7f},
    {-0.45f, -0.45f, 18.0f, 0.003f, 2.5f, 0.02f, 0.35f, 3, 1.7f},
    {-0.6f, -0.6f, 22.0f, 0.0025f, 3.0f, 0.018f, 0.4f, 3, 1.8f},

    // Fallback (origin)
    {0.0f, 0.0f, 25.0f, 0.0025f, 3.0f, 0.018f, 0.45f, 4, 1.8f}};
