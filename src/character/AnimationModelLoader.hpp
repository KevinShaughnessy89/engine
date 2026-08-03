#pragma once

#include "components/rendering/AnimationClip.hpp"
#include "rendering/MeshStructs.hpp"

struct BoneWeightData {
    std::vector<std::pair<int, float>> boneWeights;
};

struct BoneInfo {
    std::string name;
    int ID;
    int parentID;
    glm::mat4 localBindTransform;
    glm::mat4 offsetMatrix;
};

namespace AnimationModelLoader {

void size(int numVertices);
void clear();
void prepareNewMesh();
void populateAccumulator(const aiMesh* mesh);
void addBoneWeights(const aiMesh* mesh, std::vector<MeshVertex>& vertices);
std::vector<BoneInfo> buildSkeleton(const aiNode* rootNode);
AnimationClip loadClip(const aiAnimation* anim);

};  // namespace AnimationModelLoader
