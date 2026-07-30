#pragma once

// Optional per-entity distance gate for InstancedRenderable, checked by Renderer::cullInstances(). Absent means frustum-only culling; present adds a distance test so near/far LOD pairs (e.g. a tree and its billboard imposter) each only keep the instances on their side of switchDistance.
struct InstanceLOD {
    float switchDistance = 0.0f;
    bool isFarVariant = false;
};
