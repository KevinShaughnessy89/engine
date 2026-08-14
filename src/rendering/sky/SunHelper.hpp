#pragma once

#include <chrono>
#include <numbers>

#include "components/rendering/SunState.hpp"
#include "entt-main/src/entt/entt.hpp"

using Clock = std::chrono::high_resolution_clock;
using TimePoint = Clock::time_point;

class SunHelper {
   public:
    static float radius;
    static float intensity;
    static float distance;

    static void updateSunState(SunState& sunState) {
        updateSunPosition(sunState.position, sunState.elevation);
        calculateSunOrientation(sunState.elevation, sunState.direction);
        calculateSunColor(sunState.sunColor, sunState.elevation);
        calculateSkyColors(sunState.topColor, sunState.horizonColor, sunState.bottomColor,
                           sunState.skyColorAverage, sunState.position);
        calculateAmbientStrength(sunState.ambientStrength, sunState.elevation);
        sunState.intensity = intensity;
        sunState.time = calculateTimeOfDay();
    }

    static float calculateTimeOfDay() {
        return 0.7f;
        static TimePoint lastTime = Clock::now();
        static double currentTime = 100.0;
        static float dayDuration = 200.0f;

        auto now = Clock::now();
        std::chrono::duration<float, std::milli> delta = now - lastTime;
        lastTime = now;

        float deltaTimeMs = delta.count();           // milliseconds (float)
        float deltaTimeSec = deltaTimeMs / 1000.0f;  // seconds (float)

        currentTime += deltaTimeSec;

        return fmod(currentTime / dayDuration, 1.0f);
    }

    static void calculateSunOrientation(float& elevation, glm::vec3& sunDirection) {
        float timeOfDay = calculateTimeOfDay();

        float sunAngle = timeOfDay * 2.0f * 3.14159f - 3.14159f / 2.0f;

        float heightComponent = sin(sunAngle);
        float horizontalComponent = cos(sunAngle);

        sunDirection = glm::vec3(heightComponent * 0.80f, elevation, horizontalComponent * 0.3f);
    }

    static void calculateSunColor(glm::vec3& color, float elevation) {
        // Change sun color based on elevation (height in sky)
        if (elevation < -0.1f) {
            // Sun is below horizon - no contribution
            color = glm::vec3(0.0f);
            return;
        } else if (elevation < 0.1f) {
            // Sunset/sunrise colors
            float t = (elevation + 0.1f) / 0.2f;                  // 0 to 1
            glm::vec3 sunsetColor = glm::vec3(1.0f, 0.4f, 0.1f);  // Orange
            glm::vec3 dayColor = glm::vec3(1.0f, 0.95f, 0.8f);    // Warm white
            color = glm::mix(sunsetColor, dayColor, t);
            return;
        } else {
            // Daytime - warm white
            color = glm::vec3(1.0f, 0.95f, 0.8f);
            return;
        }
    }

    static void calculateAmbientStrength(float& ambientStrength, float elevation) {
        float dayAmbient = 0.5f;
        float nightAmbient = 0.35f;

        ambientStrength =
            glm::mix(nightAmbient, dayAmbient, glm::smoothstep(0.1f, 1.0f, elevation));
    }

    static void calculateSkyColors(glm::vec3& top, glm::vec3& horizon, glm::vec3& bottom,
                                   glm::vec3& average, const glm::vec3& sunPosition) {
        glm::vec3 direction = glm::normalize(sunPosition);
        float sunAngle = glm::degrees(asin(direction.y));

        if (sunAngle < -5.f) {
            // Deep night
            top = glm::vec3(0.005f, 0.005f, 0.015f);     // Almost black
            horizon = glm::vec3(0.015f, 0.015f, 0.04f);  // Dim glow
            bottom = glm::vec3(0.0f, 0.0f, 0.02f);       // Faint blue-black

        } else if (sunAngle < 0.f) {
            // Astronomical dawn/dusk
            float t = (sunAngle + 5.f) / 5.f;

            glm::vec3 nightTop = glm::vec3(0.005f, 0.005f, 0.015f);
            glm::vec3 nightHorizon = glm::vec3(0.015f, 0.015f, 0.04f);
            glm::vec3 nightBottom = glm::vec3(0.0f, 0.0f, 0.02f);

            glm::vec3 violetTop = glm::vec3(0.1f, 0.05f, 0.2f);
            glm::vec3 violetHorizon = glm::vec3(0.2f, 0.1f, 0.3f);
            glm::vec3 violetBottom = glm::vec3(0.1f, 0.0f, 0.15f);

            top = glm::mix(nightTop, violetTop, t);
            horizon = glm::mix(nightHorizon, violetHorizon, t);
            bottom = glm::mix(nightBottom, violetBottom, t);

        } else if (sunAngle < 15.f) {
            // Phase 1: Violet â†’ Fiery glow (0Â° to 15Â°)
            float t = sunAngle / 15.f;

            glm::vec3 violetTop = glm::vec3(0.1f, 0.05f, 0.2f);
            glm::vec3 violetHorizon = glm::vec3(0.2f, 0.1f, 0.3f);
            glm::vec3 violetBottom = glm::vec3(0.1f, 0.0f, 0.15f);

            glm::vec3 fireTop = glm::vec3(0.8f, 0.3f, 0.1f);
            glm::vec3 fireHorizon = glm::vec3(1.0f, 0.5f, 0.1f);
            glm::vec3 fireBottom = glm::vec3(0.6f, 0.2f, 0.1f);

            top = glm::mix(violetTop, fireTop, t);
            horizon = glm::mix(violetHorizon, fireHorizon, t);
            bottom = glm::mix(violetBottom, fireBottom, t);

        } else if (sunAngle < 30.f) {
            // Phase 2: Fiery glow â†’ Pale morning sky (15Â° to 30Â°)
            float t = (sunAngle - 15.f) / 15.f;

            glm::vec3 fireTop = glm::vec3(0.8f, 0.3f, 0.1f);
            glm::vec3 fireHorizon = glm::vec3(1.0f, 0.5f, 0.1f);
            glm::vec3 fireBottom = glm::vec3(0.6f, 0.2f, 0.1f);

            glm::vec3 morningTop = glm::vec3(0.4f, 0.65f, 1.0f);
            glm::vec3 morningHorizon = glm::vec3(0.85f, 0.9f, 1.0f);
            glm::vec3 morningBottom = glm::vec3(0.7f, 0.85f, 0.95f);

            top = glm::mix(fireTop, morningTop, t);
            horizon = glm::mix(fireHorizon, morningHorizon, t);
            bottom = glm::mix(fireBottom, morningBottom, t);
        } else {
            // Bright clear day
            top = glm::vec3(0.15f, 0.45f, 0.9f);       // Deep, saturated blue
            horizon = glm::vec3(0.55f, 0.75f, 0.95f);  // Blue-tinted (was near-white)
            bottom = glm::vec3(0.4f, 0.65f, 0.9f);     // Blue haze (was near-white)
        }

        average = (top + horizon + bottom) / 3.0f;
    }

    static void updateSunPosition(glm::vec3& sunPosition, float& elevation) {
        float timeOfDay = calculateTimeOfDay();

        float theta = timeOfDay * 2.0f * std::numbers::pi_v<float> - 3.14159f / 2;

        float a = distance;         // horizontal range (east-west)
        float b = distance * 0.8f;  // depth range (north-south), optional for tilt
        float h = distance * 0.8f;  // height of arc

        sunPosition.x = a * cos(theta);
        sunPosition.y = h * sin(theta);  // highest at noon
        sunPosition.z = b * sin(theta);

        elevation = sunPosition.y / distance;  // Normalize elevation for color calculations
    }

    static glm::mat4 calculateModelMatrix(const glm::vec3& sunPosition) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, sunPosition);
        model = glm::scale(model, glm::vec3(radius));

        return model;
    }
};

inline float SunHelper::intensity = 10.0f;
inline float SunHelper::distance = 1000.f;
inline float SunHelper::radius = 50.f;
