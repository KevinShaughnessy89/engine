#include "Model.hpp"

#include <tracy/Tracy.hpp>

#include "components/collision/ShapeData.hpp"
#include "core/Core.hpp"
#include "core/PrecompiledHeader.hpp"
#include "rendering/DisplayManager.hpp"
#include "rendering/ShaderProgram.hpp"
#include "rendering/TextureManager.hpp"

Model::Model() : modelPath(std::string("")) {
}

Model::Model(std::vector<MeshVertex> vertices, std::vector<unsigned int> indices,
             std::vector<Texture*> textures)
    : Model() {
    createMesh(vertices, indices, textures);
}

Model::Model(std::vector<MeshVertex> vertices) : Model() {
    meshes.push_back(std::make_unique<Mesh>(std::move(vertices)));
    calculateGeometricData();
}

Model::Model(std::vector<MeshVertex>&& vertices, std::vector<unsigned int>&& indices) : Model() {
    meshes.push_back(std::make_unique<Mesh>(std::move(vertices), std::move(indices)));
}

Model::Model(std::vector<MeshVertex> vertices, std::vector<unsigned int> indices, Texture* texture)
    : modelPath("") {
    if (texture == nullptr) {
        std::cout << "Warning: Texture is null for model creation." << std::endl;
        return;
    }

    std::vector<Texture*> textures{texture};
    createMesh(vertices, indices, textures);
    calculateGeometricData();
}

Model::Model(Model& other) {
    this->modelPath = other.modelPath;
    for (auto& mesh : other.meshes) {
        this->meshes.push_back(std::make_unique<Mesh>(*mesh));
    }
    this->min[0] = other.min[0];
    this->min[1] = other.min[1];
    this->min[2] = other.min[2];
    this->max[0] = other.max[0];
    this->max[1] = other.max[1];
    this->max[2] = other.max[2];
    calculateGeometricData();
}

Model::~Model() = default;

void Model::render(ShaderProgram* shader, int firstFreeTextureId, bool bindTextures) {
    ZoneScoped;
    for (auto& mesh : meshes) {
        mesh->render(shader, firstFreeTextureId, bindTextures);
    }
}

void Model::destructMesh() {
    for (auto& mesh : meshes) {
        mesh->cleanUp();
    }
}

glm::vec3 Model::getMaxCoordinates() {
    float* temp = new float[3];
    temp[0] = max[0];
    temp[1] = max[1];
    temp[2] = max[2];

    return glm::vec3(temp[0], temp[1], temp[2]);
}

glm::vec3 Model::getMinCoordinates() {
    float* temp = new float[3];
    temp[0] = min[0];
    temp[1] = min[1];
    temp[2] = min[2];

    return glm::vec3(temp[0], temp[1], temp[2]);
}

void Model::createMesh(std::vector<MeshVertex>& vertices, std::vector<unsigned int>& indices,
                       std::vector<Texture*>& textures) {
    if (std::any_of(textures.begin(), textures.end(), [](Texture* t) { return t == nullptr; })) {
        std::cout << "Warning: texture is null or textures vector is empty." << std::endl;
        return;
    }

    meshes.push_back(std::make_unique<Mesh>(std::move(vertices), std::move(indices), std::move(textures)));
}

glm::vec3 Model::calculateCentroid(const std::vector<std::unique_ptr<Mesh>>& meshes) {
    if (meshes.empty()) {
        return glm::vec3(1.0f);
    }

    // get rows for MatrixXd
    int rowCount = 0;
    for (size_t i = 0; i < meshes.size(); i++) {
        rowCount += meshes[i]->vertices.size();
    }

    // Convert vertices to Eigen format
    Eigen::MatrixXd points(rowCount, 3);
    int matrixCount = 0;
    for (size_t i = 0; i < meshes.size(); i++) {
        for (size_t j = 0; j < meshes[i]->vertices.size(); ++j) {
            points.row(matrixCount++) << meshes[i]->vertices[j].position.x,
                meshes[i]->vertices[j].position.y, meshes[i]->vertices[j].position.z;
        }
    }

    // Calculate centroid
    Eigen::Vector3d centroid = points.colwise().mean();

    return glm::vec3(static_cast<float>(centroid.x()), static_cast<float>(centroid.y()),
                     static_cast<float>(centroid.z()));
}

glm::mat3 Model::calculatePrincipalAxes(const std::vector<std::unique_ptr<Mesh>>& meshes) {
    if (meshes.empty()) {
        return glm::mat3(1.0f);
    }

    // get rows for MatrixXd
    int rowCount = 0;
    for (size_t i = 0; i < meshes.size(); i++) {
        rowCount += meshes[i]->vertices.size();
    }

    // Convert vertices to Eigen format
    Eigen::MatrixXd points(rowCount, 3);
    int matrixCount = 0;
    for (size_t i = 0; i < meshes.size(); i++) {
        for (size_t j = 0; j < meshes[i]->vertices.size(); ++j) {
            points.row(matrixCount++) << meshes[i]->vertices[j].position.x,
                meshes[i]->vertices[j].position.y, meshes[i]->vertices[j].position.z;
        }
    }

    // Calculate centroid
    Eigen::Vector3d centroid = points.colwise().mean();

    // Center the points
    points.rowwise() -= centroid.transpose();

    // Compute covariance matrix
    Eigen::Matrix3d covariance = (points.transpose() * points) / double(points.rows() - 1);

    // Perform eigendecomposition
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> eigensolver(covariance);

    // Return eigenvectors (columns are the principal axes)
    Eigen::Matrix3d principleAxes = eigensolver.eigenvectors().rowwise().reverse();

    glm::mat3 output;

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            // Note: Eigen is column-major by default, same as GLM
            output[j][i] = static_cast<float>(principleAxes(i, j));
        }
    }

    return output;
}

size_t Model::getNextID() {
    static int id = 1;
    return id++;
}

GeometricData Model::calculateGeometricData() {
    GeometricData geoData;
    geoData.min = getMinCoordinates();
    geoData.max = getMaxCoordinates();
    if (meshes.size() > 1) {
        for (int i = 0; i < meshes.size(); ++i) {
            const auto& mesh = meshes[i];
            for (const auto& vertex : mesh->vertices) {
                geoData.pointCloud.push_back(
                    quickhull::Vector3(vertex.position.x, vertex.position.y, vertex.position.z));
            }
        }
    }
    return geoData;
}

void Model::useTriangleStrips(bool value) {
    for (auto& mesh : meshes) {
        mesh->setUseTriangleStrips(value);
    }
}

void Model::generateFallbackModel() {
    auto newMesh = std::make_unique<Mesh>();
    newMesh->setCube();

    fallbackTexture = std::make_unique<Texture2D>();
    fallbackTexture->generateFallbackTexture();
    fallbackTexture->setUniform("textureDiffuse");
    newMesh->textures.push_back(fallbackTexture.get());

    meshes.push_back(std::move(newMesh));
}
