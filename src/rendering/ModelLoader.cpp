#include "ModelLoader.hpp"

#include <assimp/material.h>
#include <assimp/postprocess.h>

#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>

#include "character/AnimationModelLoader.hpp"
#include "character/AnimationRenderer.hpp"
#include "components/rendering/InstancedRenderable.hpp"
#include "components/rendering/Renderable.hpp"
#include "core/Config.hpp"
#include "core/Core.hpp"
#include "entt-main/src/entt/entt.hpp"
#include "rendering/DisplayManager.hpp"
#include "rendering/InstancedMesh.hpp"
#include "rendering/InstancedModel.hpp"
#include "rendering/Mesh.hpp"
#include "rendering/Model.hpp"
#include "rendering/ShaderType.hpp"
#include "rendering/Texture.hpp"
#include "rendering/TextureManager.hpp"

ModelLoader::ModelLoader() {
    // Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE, aiDefaultLogStream_STDOUT);
}

ModelLoader::~ModelLoader() = default;

void ModelLoader::loadScene(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open scene file " << path << std::endl;
        return;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    rapidjson::Document doc;
    doc.Parse(content.c_str(), content.length());
    if (!doc.IsObject()) {
        std::cerr << "Error: Scene file " << path << " is not a JSON object" << std::endl;
        return;
    }

    for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it) {
        const std::string name = it->name.GetString();
        const rapidjson::Value& entry = it->value;

        ModelData data;
        data.name = name;
        data.path = Config::ProjectRootDir + entry["path"].GetString();
        data.shaderType = shaderTypeFromString(entry["shaderType"].GetString());
        data.renderLayer = renderLayerFromString(entry["renderLayer"].GetString());
        data.modelType = modelTypeFromString(entry["modelType"].GetString());

        if (entry.HasMember("mergeSiblings") && entry["mergeSiblings"].GetBool()) {
            data.mergeSiblings = entry["mergeSiblings"].GetBool();
        }

        if (entry.HasMember("uniforms")) {
            data.uniforms = parseUniforms(entry["uniforms"]);
        }

        if (entry.HasMember("animations")) {
            const rapidjson::Value& anims = entry["animations"];
            for (auto animIt = anims.MemberBegin(); animIt != anims.MemberEnd(); animIt++) {
                std::string animName = animIt->name.GetString();
                std::string animPath = Config::ProjectRootDir + animIt->value.GetString();
                data.animations.push_back(std::make_pair(animName, animPath));
            }
        }

        create(name, data);

        if (!data.animations.empty()) {
            processAnimations(data);
        }
    }
}

std::vector<std::pair<std::string, std::string>> ModelLoader::parseUniforms(
    const rapidjson::Value& value) {
    std::vector<std::pair<std::string, std::string>> uniforms;
    for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it) {
        uniforms.emplace_back(it->name.GetString(), it->value.GetString());
    }
    return uniforms;
}

void ModelLoader::create(const std::string& name, ModelData& data) {
    const std::string& path = data.path;
    importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY,
                              1.0f);  // target: 1 unit = 1 meter
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
    const aiScene* scene = importer.ReadFile(
        path.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_GlobalScale |
                          aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    // Animation-only FBX exports have no meshes, which Assimp flags as an incomplete scene even
    // though the import succeeded; only bail if there's neither mesh nor animation data.
    if (!scene || !scene->mRootNode || (scene->mNumMeshes == 0 && scene->mNumAnimations == 0)) {
        std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        std::cout << "Skipping scene entry \"" << name << "\" (" << path << ")" << std::endl;
        return;
    }

    // PrintScene(scene, true);

    embeddedTextures.clear();
    if (path.size() >= 5 && path.compare(path.size() - 5, 5, ".gltf") == 0) {
        loadEmbeddedTextures(scene);
    }

    GLuint shaderID = Display.getShaderIdFromShaderType(data.shaderType);
    data.shaderID = shaderID;

    if (data.mergeSiblings) {
        if (data.modelType == ModelType::Single) {
            auto modelPtr = std::make_unique<Model>();
            createModel(data, std::move(modelPtr), scene, scene->mRootNode);
        }
        if (data.modelType == ModelType::Instanced) {
            auto instancedModelPtr = std::make_unique<InstancedModel>();
            createInstancedModel(data, std::move(instancedModelPtr), scene, scene->mRootNode);
        }
    } else {
        for (unsigned int i = 0; i < scene->mRootNode->mNumChildren; i++) {
            aiNode* node = scene->mRootNode->mChildren[i];
            if (data.modelType == ModelType::Single) {
                auto modelPtr = std::make_unique<Model>();
                createModel(data, std::move(modelPtr), scene, node);
            }
            if (data.modelType == ModelType::Instanced) {
                auto instancedModelPtr = std::make_unique<InstancedModel>();
                createInstancedModel(data, std::move(instancedModelPtr), scene, node);
            }
        }
    }
    return;
}

void ModelLoader::createModel(ModelData& data, std::unique_ptr<Model> modelPtr,
                              const aiScene* scene, aiNode* node) {
    modelPtr->modelPath = data.path;
    modelPtr->name = data.name;
    modelPtr->directory = modelPtr->modelPath.substr(0, modelPtr->modelPath.find_last_of('/'));
    materialCache.clear();
    AnimationModelLoader::clear();

    processNode(modelPtr.get(), node, scene);

    if (!modelPtr->meshes.empty()) {
        auto e = Registry.create();
        data.entity = e;
        Registry.emplace<Renderable>(e, modelPtr.get(), data.shaderID, data.renderLayer, true);
        std::vector<BoneInfo> skeleton = AnimationModelLoader::buildSkeleton(scene->mRootNode);
        Registry.emplace<SkeletonComponent>(e, skeleton);

        entities[data.name].push_back(e);

        models.push_back(std::move(modelPtr));
    }
}

void ModelLoader::createInstancedModel(ModelData& data, std::unique_ptr<InstancedModel> modelPtr,
                                       const aiScene* scene, aiNode* node) {
    modelPtr->modelPath = data.path;
    modelPtr->name = data.name;
    modelPtr->directory = data.path.substr(0, data.path.find_last_of('/'));
    materialCache.clear();
    processNode(modelPtr.get(), node, scene);

    if (!modelPtr->meshes.empty()) {
        auto e = Registry.create();
        data.entity = e;
        Registry.emplace<InstancedRenderable>(e, modelPtr.get(), data.shaderID, data.renderLayer,
                                              true);
        std::vector<BoneInfo> skeleton = AnimationModelLoader::buildSkeleton(scene->mRootNode);
        Registry.emplace<SkeletonComponent>(e, skeleton);

        entities[data.name].push_back(e);

        instancedModels.push_back(std::move(modelPtr));
    }
}

template <typename ModelT>
void ModelLoader::processNode(ModelT* model, aiNode* node, const aiScene* scene) {
    // process each mesh located at the current node
    glm::mat4 transform = glm::transpose(glm::make_mat4(&node->mTransformation.a1));
    model->transform = transform;
    // Correct matrix for normals/tangents under non-uniform scale:
    glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(transform)));
    if (node->mNumMeshes > 0) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            processMesh(model, mesh, scene, transform, normalMatrix);
        }
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the
    // children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(model, node->mChildren[i], scene);
    }
}

template <typename ModelT>
void ModelLoader::processMesh(ModelT* model, aiMesh* mesh, const aiScene* scene,
                              const glm::mat4& transform, const glm::mat3& normalMatrix) {
    std::vector<MeshVertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture*> textures;

    if (mesh->HasBones()) {
        AnimationModelLoader::prepareNewMesh();
        AnimationModelLoader::size(mesh->mNumVertices);
        AnimationModelLoader::populateAccumulator(mesh);
    }

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        MeshVertex vertex;
        glm::vec3 vector;

        // positions
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;

        vertex.position = transform * glm::vec4(vector, 1.0);

        // Get OBB values (min/max on each axis), in the same transformed space as
        // vertex.position
        if (model->min[0] == std::numeric_limits<float>::infinity()) {
            model->min[0] = model->max[0] = vertex.position.x;
            model->min[1] = model->max[1] = vertex.position.y;
            model->min[2] = model->max[2] = vertex.position.z;
        } else {
            // Update min/max for subsequent vertices
            model->min[0] = std::min(model->min[0], vertex.position.x);
            model->max[0] = std::max(model->max[0], vertex.position.x);
            model->min[1] = std::min(model->min[1], vertex.position.y);
            model->max[1] = std::max(model->max[1], vertex.position.y);
            model->min[2] = std::min(model->min[2], vertex.position.z);
            model->max[2] = std::max(model->max[2], vertex.position.z);
        }
        // normals
        if (mesh->HasNormals()) {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.normal = glm::normalize(normalMatrix * vector);
        }
        if (mesh->mTextureCoords[0])  // does the mesh contain texture coordinates?
        {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.texCoords = vec;
            // tangent
            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vertex.tangent = glm::normalize(normalMatrix * vector);
            // bitangent
            vector.x = mesh->mBitangents[i].x;
            vector.y = mesh->mBitangents[i].y;
            vector.z = mesh->mBitangents[i].z;
            vertex.bitangent = glm::normalize(normalMatrix * vector);
        } else
            vertex.texCoords = glm::vec2(0.0f, 0.0f);

        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) indices.push_back(face.mIndices[j]);
    }

    // Bones yo
    if (mesh->HasBones()) AnimationModelLoader::addBoneWeights(mesh, vertices);

    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    aiString name;
    material->Get(AI_MATKEY_NAME, name);
    std::string materialName = name.C_Str();
    size_t materialIndex = getOrCreateMaterialIndex(model, material, mesh->mMaterialIndex);

    static const std::vector<std::pair<aiTextureType, const char*>> allTypes = {
        {aiTextureType_DIFFUSE, "textureDiffuse"},
        {aiTextureType_NORMALS, "textureNormal"},
        {aiTextureType_METALNESS, "textureMetallic"},
        {aiTextureType_DIFFUSE_ROUGHNESS, "textureRoughness"},
        {aiTextureType_OPACITY, "textureAlpha"},
        {aiTextureType_AMBIENT_OCCLUSION, "textureAO"}};

    for (const auto& [textureType, uniform] : allTypes) {
        for (Texture* tex : loadMaterialTextures(material, textureType)) {
            if (tex) {
                tex->setUniform(uniform);
                textures.push_back(tex);
            }
        }
    }

    createMesh(model, vertices, indices, textures, materialName, materialIndex);
}

void ModelLoader::createMesh(Model* model, std::vector<MeshVertex>& vertices,
                             std::vector<unsigned int>& indices, std::vector<Texture*>& textures,
                             std::string material, size_t materialIndex) {
    if (std::any_of(textures.begin(), textures.end(), [](Texture* t) { return t == nullptr; })) {
        std::cout << "Warning: a texture for material \"" << material << "\" is null." << std::endl;
    }
    if (textures.empty()) {
        std::cout << "Warning: no textures resolved for material \"" << material << "\"."
                  << std::endl;
    }

    auto mesh =
        std::make_unique<Mesh>(std::move(vertices), std::move(indices), std::move(textures));
    mesh->materialIndex = static_cast<int>(materialIndex);
    model->meshes.push_back(std::move(mesh));
}

void ModelLoader::createMesh(InstancedModel* model, std::vector<MeshVertex>& vertices,
                             std::vector<unsigned int>& indices, std::vector<Texture*>& textures,
                             std::string material, size_t materialIndex) {
    if (std::any_of(textures.begin(), textures.end(), [](Texture* t) { return t == nullptr; })) {
        std::cout << "Warning: a texture for material \"" << material << "\" is null." << std::endl;
        return;
    }
    if (textures.empty()) {
        std::cout << "Warning: no textures resolved for material \"" << material << "\"."
                  << std::endl;
    }

    auto mesh = InstancedMesh(std::move(vertices), std::move(indices), std::move(textures));
    mesh.material = material;
    mesh.materialIndex = static_cast<int>(materialIndex);
    model->meshes.push_back(std::move(mesh));
}

void ModelLoader::processAnimations(const ModelData& data) {
    auto& registryEntry = AnimationRenderer::registry[data.name];

    for (const auto& [animName, animPath] : data.animations) {
        auto* scene = importer.ReadFile(animPath.c_str(), aiProcess_GlobalScale);
        if (!scene) {
            std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            std::cout << "Skipping animation \"" << animName << "\" (" << animPath << ")"
                      << std::endl;
            continue;
        }
        for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
            const aiAnimation* anim = scene->mAnimations[i];
            std::cout << "Anim " << i << " raw name: '" << anim->mName.C_Str() << "'\n";
            AnimationClip clip = AnimationModelLoader::loadClip(anim);
            clip.name = animName;  // use the JSON key, not the FBX's internal track name
            registryEntry.clips.push_back(clip);
            registryEntry.nameToID[clip.name] = static_cast<int>(registryEntry.clips.size() - 1);
        }
    }

    auto& animationState = Registry.emplace<AnimationState>(data.entity);
    animationState.clips = registryEntry.clips;
    animationState.nameToID = registryEntry.nameToID;
}

Material ModelLoader::extractMaterial(aiMaterial* material) {
    Material mat;

    int shadingModel = 0;
    if (material->Get(AI_MATKEY_SHADING_MODEL, shadingModel) == AI_SUCCESS)
        mat.shadingModel = shadingModel;

    aiColor3D color;
    if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
        mat.diffuse = glm::vec3(color.r, color.g, color.b);
    if (material->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS)
        mat.emissive = glm::vec3(color.r, color.g, color.b);
    if (material->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS)
        mat.ambient = glm::vec3(color.r, color.g, color.b);
    if (material->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
        mat.specular = glm::vec3(color.r, color.g, color.b);
    if (material->Get(AI_MATKEY_COLOR_TRANSPARENT, color) == AI_SUCCESS)
        mat.transparent = glm::vec3(color.r, color.g, color.b);
    if (material->Get(AI_MATKEY_COLOR_REFLECTIVE, color) == AI_SUCCESS)
        mat.reflective = glm::vec3(color.r, color.g, color.b);

    material->Get(AI_MATKEY_SHININESS_STRENGTH, mat.shininessPercent);
    material->Get(AI_MATKEY_SHININESS, mat.shininess);
    material->Get(AI_MATKEY_ROUGHNESS_FACTOR, mat.roughness);
    material->Get(AI_MATKEY_TRANSPARENCYFACTOR, mat.transparencyFactor);
    material->Get(AI_MATKEY_OPACITY, mat.opacity);
    material->Get(AI_MATKEY_REFLECTIVITY, mat.reflectivity);
    material->Get(AI_MATKEY_BUMPSCALING, mat.bumpScaling);
    material->Get("$mat.displacementscaling", 0, 0, mat.displacementScaling);

    return mat;
}

template <typename ModelT>
size_t ModelLoader::getOrCreateMaterialIndex(ModelT* model, aiMaterial* material,
                                             unsigned int sceneMaterialIndex) {
    auto it = materialCache.find(sceneMaterialIndex);
    if (it != materialCache.end()) return it->second;

    model->materials.push_back(extractMaterial(material));
    size_t index = model->materials.size() - 1;
    materialCache[sceneMaterialIndex] = index;
    return index;
}

std::vector<Texture*> ModelLoader::loadMaterialTextures(aiMaterial* mat, aiTextureType type) {
    std::vector<Texture*> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        std::cout << "Loading texture: " << str.C_Str() << std::endl;

        Texture* texture = nullptr;
        if (str.length > 0 && str.C_Str()[0] == '*') {
            unsigned int embeddedIndex = static_cast<unsigned int>(std::atoi(str.C_Str() + 1));
            auto it = embeddedTextures.find(embeddedIndex);
            if (it != embeddedTextures.end()) texture = it->second;
        } else {
            texture = Textures.getTexture(str.C_Str());
        }

        if (texture != nullptr) textures.push_back(texture);
    }
    return textures;
}

void ModelLoader::loadEmbeddedTextures(const aiScene* scene) {
    for (unsigned int i = 0; i < scene->mNumTextures; i++) {
        Texture* texture = createTextureFromEmbedded(scene->mTextures[i], i);
        if (texture != nullptr) embeddedTextures[i] = texture;
    }
}

Texture* ModelLoader::createTextureFromEmbedded(const aiTexture* tex, unsigned int index) {
    std::string name = "embedded_" + std::to_string(index);

    unsigned char* decoded = nullptr;
    int width = 0, height = 0, channels = 0;

    if (tex->mHeight == 0) {
        // Compressed image (png/jpg/etc) stored as a raw byte buffer of length mWidth.
        decoded = stbi_load_from_memory(reinterpret_cast<unsigned char*>(tex->pcData), tex->mWidth,
                                        &width, &height, &channels, 0);
        if (!decoded) {
            std::cout << "Warning: failed to decode embedded texture \"" << name << "\"."
                      << std::endl;
            return nullptr;
        }
    } else {
        // Uncompressed BGRA8888 texels; convert to RGBA for glTexImage2D.
        width = tex->mWidth;
        height = tex->mHeight;
        channels = 4;
        decoded = new unsigned char[width * height * 4];
        for (int p = 0; p < width * height; p++) {
            const aiTexel& texel = tex->pcData[p];
            decoded[p * 4 + 0] = texel.r;
            decoded[p * 4 + 1] = texel.g;
            decoded[p * 4 + 2] = texel.b;
            decoded[p * 4 + 3] = texel.a;
        }
    }

    // wasCached=true routes cleanup through delete[] (matching the `new[]` above) instead of
    // stbi_image_free, which only applies to the malloc'd buffer stbi_load_from_memory returns.
    bool wasCached = tex->mHeight != 0;
    TextureData* textureData =
        new TextureData(name, name, width, height, channels, decoded, wasCached);
    return new Texture2D(std::vector<TextureData*>({textureData}), name);
}

Renderable& ModelLoader::getRenderable(const std::string& name, int index) {
    if (entities[name].empty()) {
        throw("ERROR: no renderables at key " + name + "\n");
    }
    auto* renderable = Registry.try_get<Renderable>(entities[name][index]);

    if (renderable)
        return *renderable;
    else
        throw("ERROR: no renderable at key " + name + " index " + std::to_string(index) + "\n");
}

InstancedRenderable& ModelLoader::getInstancedRenderable(const std::string& name, int index) {
    if (entities[name].empty()) {
        throw("ERROR: no instanced renderables at key " + name + "\n");
    }
    auto* instancedRenderable = Registry.try_get<InstancedRenderable>(entities[name][index]);

    if (instancedRenderable)
        return *instancedRenderable;
    else
        throw("ERROR: no instanced renderable at key " + name + " index " + std::to_string(index) +
              "\n");
}

Model* ModelLoader::getModel(const std::string& name, int index) {
    if (index < 0 || index >= models.size()) {
        throw("ERROR: model index out of bounds for key " + name + "\n");
    }
    auto* model = entities[name].empty()
                      ? nullptr
                      : Registry.try_get<Renderable>(entities[name][index])->model;
    if (model)
        return model;
    else
        throw("ERROR: no model at key " + name + " index " + std::to_string(index) + "\n");
}

InstancedModel* ModelLoader::getInstancedModel(const std::string& name, int index) {
    if (index < 0 || index >= instancedModels.size()) {
        throw("ERROR: instanced model index out of bounds for key " + name + "\n");
    }
    auto* instancedModel =
        entities[name].empty()
            ? nullptr
            : Registry.try_get<InstancedRenderable>(entities[name][index])->model;
    if (instancedModel)
        return instancedModel;
    else
        throw("ERROR: no instanced model at key " + name + " index " + std::to_string(index) +
              "\n");
}

entt::entity ModelLoader::getEntity(const std::string& name, int index) {
    if (index < 0 || index >= entities[name].size()) {
        throw("ERROR: entity index out of bounds for key " + name + "\n");
    }
    entt::entity e = entities[name][index];

    if (Registry.valid(e))
        return e;
    else
        throw("ERROR: no entity at key " + name + " index " + std::to_string(index) + "\n");
}

void ModelLoader::PrintMetadataEntry(const aiMetadata* meta, unsigned i) {
    const std::string key = meta->mKeys[i].C_Str();
    const aiMetadataEntry& entry = meta->mValues[i];

    std::cout << "    " << key << " = ";
    switch (entry.mType) {
        case AI_BOOL:
            std::cout << (*static_cast<bool*>(entry.mData) ? "true" : "false");
            break;
        case AI_INT32:
            std::cout << *static_cast<int32_t*>(entry.mData);
            break;
        case AI_UINT64:
            std::cout << *static_cast<uint64_t*>(entry.mData);
            break;
        case AI_FLOAT:
            std::cout << *static_cast<float*>(entry.mData);
            break;
        case AI_DOUBLE:
            std::cout << *static_cast<double*>(entry.mData);
            break;
        case AI_AISTRING:
            std::cout << static_cast<aiString*>(entry.mData)->C_Str();
            break;
        case AI_AIVECTOR3D: {
            auto* v = static_cast<aiVector3D*>(entry.mData);
            std::cout << "(" << v->x << ", " << v->y << ", " << v->z << ")";
            break;
        }
        default:
            std::cout << "<unhandled metadata type: " << entry.mType << ">";
    }
    std::cout << "\n";
}

void ModelLoader::PrintSceneMetadata(const aiScene* scene) {
    std::cout << "=== Scene Metadata ===\n";
    if (!scene->mMetaData || scene->mMetaData->mNumProperties == 0) {
        std::cout << "  (none)\n";
        return;
    }
    for (unsigned i = 0; i < scene->mMetaData->mNumProperties; ++i) {
        PrintMetadataEntry(scene->mMetaData, i);
    }
}

void ModelLoader::PrintMaterialSummary(const aiScene* scene) {
    std::cout << "=== Embedded Textures (" << scene->mNumTextures << ") ===\n";
    for (unsigned int i = 0; i < scene->mNumTextures; i++) {
        const aiTexture* tex = scene->mTextures[i];
        std::cout << "  [" << i << "] filename hint=\"" << tex->mFilename.C_Str() << "\""
                  << "  formatHint=\"" << tex->achFormatHint << "\""
                  << "  width=" << tex->mWidth << "  height=" << tex->mHeight
                  << (tex->mHeight == 0 ? "  (compressed, mWidth = byte count)"
                                        : "  (raw RGBA texel data)")
                  << "\n";
    }

    std::cout << "=== Materials (" << scene->mNumMaterials << ") ===\n";
    for (unsigned i = 0; i < scene->mNumMaterials; ++i) {
        aiString name;
        aiMaterial* material = scene->mMaterials[i];
        std::cout << "=== Raw properties for material \"" << material->GetName().C_Str() << "\" ("
                  << material->mNumProperties << " properties) ===\n";

        for (unsigned int i = 0; i < material->mNumProperties; i++) {
            const aiMaterialProperty* prop = material->mProperties[i];

            std::cout << "  [" << i << "] key=\"" << prop->mKey.C_Str() << "\""
                      << "  semantic=" << prop->mSemantic << "  index=" << prop->mIndex
                      << "  dataLength=" << prop->mDataLength << "  type=";

            switch (prop->mType) {
                case aiPTI_Float:
                    std::cout << "Float value=";
                    if (prop->mDataLength >= sizeof(float))
                        std::cout << *reinterpret_cast<float*>(prop->mData);
                    break;
                case aiPTI_Double:
                    std::cout << "Double value=";
                    if (prop->mDataLength >= sizeof(double))
                        std::cout << *reinterpret_cast<double*>(prop->mData);
                    break;
                case aiPTI_String: {
                    // aiString is stored with a length prefix (uint32_t) then bytes
                    std::cout << "String value=";
                    const aiString* str = reinterpret_cast<const aiString*>(prop->mData);
                    std::cout << "\"" << str->C_Str() << "\"";
                    break;
                }
                case aiPTI_Integer:
                    std::cout << "Integer value=";
                    if (prop->mDataLength >= sizeof(int))
                        std::cout << *reinterpret_cast<int*>(prop->mData);
                    break;
                case aiPTI_Buffer:
                    std::cout << "Buffer (raw bytes, length " << prop->mDataLength << ")";
                    break;
                default:
                    std::cout << "unknown(" << prop->mType << ")";
            }
            std::cout << "\n";
        }
    }
}

void ModelLoader::PrintMeshSummary(const aiScene* scene, bool detailed) {
    std::cout << "=== Meshes (" << scene->mNumMeshes << ") ===\n";
    for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[i];
        std::cout << "  [" << i << "] \"" << mesh->mName.C_Str() << "\""
                  << "  verts=" << mesh->mNumVertices << "  faces=" << mesh->mNumFaces
                  << "  material=" << mesh->mMaterialIndex << "\n";

        if (!detailed) continue;

        // AABB — the single most useful number for catching scale bugs like the one you just
        // hit
        aiVector3D mn(FLT_MAX, FLT_MAX, FLT_MAX), mx(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        for (unsigned v = 0; v < mesh->mNumVertices; ++v) {
            const aiVector3D& p = mesh->mVertices[v];
            mn.x = std::min(mn.x, p.x);
            mn.y = std::min(mn.y, p.y);
            mn.z = std::min(mn.z, p.z);
            mx.x = std::max(mx.x, p.x);
            mx.y = std::max(mx.y, p.y);
            mx.z = std::max(mx.z, p.z);
        }
        aiVector3D extent = mx - mn;
        std::cout << "      AABB min(" << mn.x << ", " << mn.y << ", " << mn.z << ")"
                  << " max(" << mx.x << ", " << mx.y << ", " << mx.z << ")"
                  << " extent(" << extent.x << ", " << extent.y << ", " << extent.z << ")\n";

        std::cout << "      hasNormals=" << (mesh->HasNormals() ? "y" : "n")
                  << " hasUVs=" << (mesh->HasTextureCoords(0) ? "y" : "n")
                  << " hasTangents=" << (mesh->HasTangentsAndBitangents() ? "y" : "n")
                  << " hasBones=" << mesh->mNumBones << "\n";
    }
}

void ModelLoader::PrintNode(const aiNode* node, int depth, bool detailed) {
    std::string indent(depth * 2, ' ');
    std::cout << indent << node->mName.C_Str() << " (meshes: " << node->mNumMeshes;

    if (node->mNumMeshes > 0) {
        std::cout << " -> [";
        for (unsigned i = 0; i < node->mNumMeshes; ++i) {
            std::cout << node->mMeshes[i];
            if (i + 1 < node->mNumMeshes) std::cout << ", ";
        }
        std::cout << "]";
    }
    std::cout << ")\n";

    if (detailed) {
        const aiMatrix4x4& t = node->mTransformation;
        std::cout << indent << "  transform: "
                  << "[" << t.a1 << " " << t.a2 << " " << t.a3 << " " << t.a4 << "]  "
                  << "[" << t.b1 << " " << t.b2 << " " << t.b3 << " " << t.b4 << "]  "
                  << "[" << t.c1 << " " << t.c2 << " " << t.c3 << " " << t.c4 << "]\n";

        // Decompose so you don't have to mentally parse a raw 4x4 for scale/rotation issues
        aiVector3D scale, rotationEuler, translation;
        aiQuaternion rotation;
        t.Decompose(scale, rotation, translation);
        std::cout << indent << "  scale(" << scale.x << ", " << scale.y << ", " << scale.z << ")"
                  << "  translation(" << translation.x << ", " << translation.y << ", "
                  << translation.z << ")\n";
    }

    for (unsigned i = 0; i < node->mNumChildren; ++i)
        PrintNode(node->mChildren[i], depth + 1, detailed);
}

void ModelLoader::PrintScene(const aiScene* scene, bool detailed) {
    std::cout << "\n########## Assimp Scene Dump ##########\n";
    PrintSceneMetadata(scene);
    if (detailed) PrintMaterialSummary(scene);
    PrintMeshSummary(scene, detailed);
    std::cout << "=== Node Hierarchy ===\n";
    PrintNode(scene->mRootNode, 0, detailed);
    std::cout << "########################################\n\n";
}
