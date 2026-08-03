#include "character/AnimationModelLoader.hpp"

namespace {

std::string stripMaximoBullshit(const std::string& name) {
    auto pos = name.find(":");
    return pos == std::string::npos ? name : name.substr(pos + 1);
}

glm::mat4 AssimpToGlm(const aiMatrix4x4 aiMatrix) {
    glm::mat4 glmMatrix;

    glmMatrix[0][0] = aiMatrix.a1;
    glmMatrix[1][0] = aiMatrix.a2;
    glmMatrix[2][0] = aiMatrix.a3;
    glmMatrix[3][0] = aiMatrix.a4;
    glmMatrix[0][1] = aiMatrix.b1;
    glmMatrix[1][1] = aiMatrix.b2;
    glmMatrix[2][1] = aiMatrix.b3;
    glmMatrix[3][1] = aiMatrix.b4;
    glmMatrix[0][2] = aiMatrix.c1;
    glmMatrix[1][2] = aiMatrix.c2;
    glmMatrix[2][2] = aiMatrix.c3;
    glmMatrix[3][2] = aiMatrix.c4;
    glmMatrix[0][3] = aiMatrix.d1;
    glmMatrix[1][3] = aiMatrix.d2;
    glmMatrix[2][3] = aiMatrix.d3;
    glmMatrix[3][3] = aiMatrix.d4;

    return glmMatrix;
}

std::vector<BoneWeightData> tempBoneData;
std::unordered_map<std::string, int> boneNameToID;
std::vector<glm::mat4> boneOffsets;
const int MAX_BONES_PER_VERTEX = 4;
int numVertices;
std::vector<BoneInfo> skeleton;

void processNode(const aiNode* node, int parentID, std::vector<BoneInfo>& skeleton) {
    std::string boneName = stripMaximoBullshit(node->mName.C_Str());
    auto it = boneNameToID.find(boneName);
    int currentParent = parentID;

    if (it != boneNameToID.end()) {
        int boneID = it->second;
        skeleton[boneID].name = boneName;
        skeleton[boneID].ID = boneID;
        skeleton[boneID].parentID = parentID;
        skeleton[boneID].localBindTransform = AssimpToGlm(node->mTransformation);
        skeleton[boneID].offsetMatrix = boneOffsets[boneID];
        currentParent = boneID;
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], currentParent, skeleton);
    }
}

}  // namespace

namespace AnimationModelLoader {

void size(int numVertices) {
    ::numVertices = numVertices;
    tempBoneData.resize(numVertices);
}

void clear() {
    tempBoneData.clear();
    boneNameToID.clear();
    boneOffsets.clear();
}

void prepareNewMesh() {
    tempBoneData.clear();
}

void populateAccumulator(const aiMesh* mesh) {
    for (unsigned int i = 0; i < mesh->mNumBones; i++) {
        aiBone* bone = mesh->mBones[i];
        std::string boneName = stripMaximoBullshit(bone->mName.C_Str());

        int boneID;

        auto it = boneNameToID.find(boneName);
        if (it == boneNameToID.end()) {
            boneID = static_cast<int>(boneOffsets.size());
            boneNameToID[boneName] = boneID;
            boneOffsets.push_back(AssimpToGlm(bone->mOffsetMatrix));
        } else {
            boneID = it->second;
        }

        for (int j = 0; j < bone->mNumWeights; j++) {
            unsigned int vertexID = bone->mWeights[j].mVertexId;
            float weight = bone->mWeights[j].mWeight;
            tempBoneData[vertexID].boneWeights.push_back({boneID, weight});
        }
    }
}

void addBoneWeights(const aiMesh* mesh, std::vector<MeshVertex>& vertices) {
    for (int i = 0; i < numVertices; i++) {
        auto& weights = tempBoneData[i].boneWeights;

        std::sort(weights.begin(), weights.end(),
                  [](auto& a, auto& b) { return a.second > b.second; });

        if (weights.size() > MAX_BONES_PER_VERTEX) {
            weights.resize(MAX_BONES_PER_VERTEX);
        }

        float sum = 0.0f;
        for (auto& [idx, weight] : weights) sum += weight;

        glm::ivec4 boneIDs(0);
        glm::vec4 boneWeights(0.0f);

        for (int j = 0; j < weights.size(); j++) {
            boneIDs[j] = weights[j].first;
            boneWeights[j] = weights[j].second / sum;
        }

        vertices[i].boneIDs = boneIDs;
        vertices[i].boneWeights = boneWeights;

        if (weights.empty()) {
            std::cout << "Warning: Vertex " << i << " has no bone weights assigned." << std::endl;
        }

        if (boneWeights.x + boneWeights.y + boneWeights.z + boneWeights.w > 1.0f + 1e-3f) {
            std::cout << "Warning: Vertex " << i << " has bone weights that sum to more than 1."
                      << std::endl;
        }

        if (boneWeights.x == 0 && boneWeights.y == 0 && boneWeights.z == 0 && boneWeights.w == 0) {
            std::cout << "Warning: Vertex " << i << " has bone weights that sum to zero."
                      << std::endl;
        }
    }
}

std::vector<BoneInfo> buildSkeleton(const aiNode* rootNode) {
    std::vector<BoneInfo> skeleton(boneNameToID.size());
    processNode(rootNode, -1, skeleton);
    return skeleton;
}

AnimationClip loadClip(const aiAnimation* anim) {
    AnimationClip clip;
    clip.name = anim->mName.C_Str();
    clip.ticksPerSecond = (anim->mTicksPerSecond != 0) ? anim->mTicksPerSecond : 25.0f;
    clip.duration = anim->mDuration / clip.ticksPerSecond;  // convert ticks -> seconds

    for (unsigned int c = 0; c < anim->mNumChannels; c++) {
        aiNodeAnim* nodeAnim = anim->mChannels[c];
        AnimationChannel channel;
        channel.boneName = stripMaximoBullshit(nodeAnim->mNodeName.C_Str());

        for (unsigned int k = 0; k < nodeAnim->mNumPositionKeys; k++) {
            auto& key = nodeAnim->mPositionKeys[k];
            channel.positionKeys.push_back({static_cast<float>(key.mTime / clip.ticksPerSecond),
                                            glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z)});
        }
        for (unsigned int k = 0; k < nodeAnim->mNumRotationKeys; k++) {
            auto& key = nodeAnim->mRotationKeys[k];
            channel.rotationKeys.push_back({
                static_cast<float>(key.mTime / clip.ticksPerSecond),
                glm::quat(key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z)  // note: w first
            });
        }
        for (unsigned int k = 0; k < nodeAnim->mNumScalingKeys; k++) {
            auto& key = nodeAnim->mScalingKeys[k];
            channel.scaleKeys.push_back({static_cast<float>(key.mTime / clip.ticksPerSecond),
                                         glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z)});
        }

        clip.channels[channel.boneName] = channel;
    }

    return clip;
}

};  // namespace AnimationModelLoader
