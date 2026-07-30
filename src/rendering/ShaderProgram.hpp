#include <string>

#include "core/PrecompiledHeader.hpp"

#ifndef _SHADERPROGRAM
#define _SHADERPROGRAM

class ShaderProgram {
   public:
    GLuint ID;
    std::unordered_map<std::string, GLint> uniformLocationCache;
    ShaderProgram() {}
    ShaderProgram(std::string vertexPath, std::string fragmentPath,
                  std::string tessControlPath = "", std::string tessEvaluationPath = "");
    GLint getUniformLocation(const char* name);
    void cacheActiveUniforms();
    void use();
    void setBool(const std::string& name, bool value);
    void setInt(const std::string& name, int value);
    void setFloat(const std::string& name, float value);
    void setVec2(const std::string& name, const glm::vec2& value);
    void setVec2(const std::string& name, float x, float y);
    void setVec3(const std::string& name, const glm::vec3& value);
    void setVec3(const std::string& name, float x, float y, float z);
    void setVec4(const std::string& name, const glm::vec4& value);
    void setVec4(const std::string& name, float x, float y, float z, float w);
    void setMat2(const std::string& name, const glm::mat2& mat);
    void setMat3(const std::string& name, const glm::mat3& mat);
    void setMat4(const std::string& name, const glm::mat4& mat);

    // const char* overloads: avoid constructing a temporary std::string on the hot uniform-update
    // path, where names come from static string literals (e.g. via UniformName::toCString).
    void setInt(const char* name, int value);
    void setFloat(const char* name, float value);
    void setVec2(const char* name, const glm::vec2& value);
    void setVec3(const char* name, const glm::vec3& value);
    void setVec4(const char* name, const glm::vec4& value);
    void setMat3(const char* name, const glm::mat3& mat);
    void setMat4(const char* name, const glm::mat4& mat);

   private:
    void checkCompileErrors(GLuint shader, std::string type);
};

#endif
