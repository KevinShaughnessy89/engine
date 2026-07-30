#pragma once
#include "core/PrecompiledHeader.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define RAPIDJSON_ASSERT(x) // no-op to disable rapidjson assertions
#include <rapidjson/document.h>

#include <Eigen/Dense>
#include <algorithm>
#include <any>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/vec3.hpp>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "assimp/Importer.hpp"
#include "assimp/mesh.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "core/Config.hpp"
#include "rendering/AssetFilePath.hpp"
#include "stbi/stb_image.h"

inline std::ostream& operator<<(std::ostream& os, const glm::vec3& v) {
    return os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
}

inline std::ostream& operator<<(std::ostream& os, const glm::mat4& mat) {
    os << "mat4(\n";
    for (int row = 0; row < 4; ++row) {
        os << "  ";
        for (int col = 0; col < 4; ++col) {
            os << mat[col][row];  // GLM is column-major
            if (col < 3) os << ", ";
        }
        os << "\n";
    }
    os << ")";
    return os;
}
