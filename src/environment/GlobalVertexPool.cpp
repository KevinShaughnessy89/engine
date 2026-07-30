#include "core/PrecompiledHeader.hpp"
// #include <glm/geometric.hpp>     // for normalize
// #include <glm/gtc/type_ptr.hpp>  // if needed for Vec conversions

// #include "GlobalVertexPool.hpp"
// #include "rendering/MeshStructs.hpp"

// GlobalVertexPool::GlobalVertexPool(float eps) : epsilon(eps) {
// }

// WeldKey GlobalVertexPool::quantize(const glm::vec4& worldPosition) const {
//     return WeldKey{static_cast<int64_t>(std::floor(worldPosition.x / epsilon)),
//                    static_cast<int64_t>(std::floor(worldPosition.y / epsilon)),
//                    static_cast<int64_t>(std::floor(worldPosition.z / epsilon)), 0, 0};
// }

// unsigned int GlobalVertexPool::addVertex(const MeshVertex& meshVertex) {
//     std::lock_guard<std::mutex> lock(
//         welderMutex);  // Many threads will contend for this, may become a bottleneck

//     WeldKey newKey = quantize(meshVertex.position);

//     auto it = snappedPositions.find(newKey);
//     if (it != snappedPositions.end()) {
//         int existingIndex = static_cast<unsigned int>(it->second);
//         meshData.globalIndices.push_back(static_cast<unsigned int>(existingIndex));

//         return existingIndex;
//     }

//     // Convert back to world coordinates (snapped)
//     glm::vec4 snappedVertex(static_cast<float>(newKey.idx) * epsilon,
//                             static_cast<float>(newKey.idy) * epsilon,
//                             static_cast<float>(newKey.idz) * epsilon, 1.f);

//     MeshVertex newVertex = meshVertex;  // copy all attributes
//     newVertex.position = snappedVertex;

//     unsigned int newIndex = static_cast<unsigned int>(meshData.vertices.size());
//     snappedPositions.try_emplace(newKey, static_cast<unsigned int>(newIndex));
//     meshData.vertices.emplace_back(newVertex);
//     return newIndex;
// }

// void GlobalVertexPool::addVertexRemap(int worldX, int worldZ, unsigned int globalIndex) {
//     std::lock_guard<std::mutex> lock(welderMutex);
//     vertexRemap[glm::vec2(worldX, worldZ)] = globalIndex;
// }

// unsigned int GlobalVertexPool::getVertexRemap(int worldX, int worldZ) {
//     std::lock_guard<std::mutex> lock(welderMutex);
//     return vertexRemap[glm::vec2(worldX, worldZ)];
// }

// MeshVertex GlobalVertexPool::getVertex(const int index) {
//     std::lock_guard<std::mutex> lock(welderMutex);
//     if (index < 0 || static_cast<size_t>(index) >= meshData.vertices.size()) {
//         std::cerr << "Attempting to access out of bounds unique vertex." << std::endl;
//         return MeshVertex();
//     }

//     return meshData.vertices[index];
// }

// void GlobalVertexPool::clear() {
//     snappedPositions.clear();
//     meshData.vertices.clear();
//     meshData.globalIndices.clear();
// }

// std::vector<MeshVertex>& GlobalVertexPool::getGlobalVertices() {
//     return meshData.vertices;
// }

// std::vector<unsigned int>& GlobalVertexPool::getGlobalIndices() {
//     return meshData.globalIndices;
// }
