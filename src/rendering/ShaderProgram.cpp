#include "ShaderProgram.hpp"

#include <fstream>

#include "UniformName.hpp"
#include "core/PrecompiledHeader.hpp"

ShaderProgram::ShaderProgram(std::string vertexPath, std::string fragmentPath,
                             std::string tessControlPath, std::string tessEvaluationPath) {
    // 1. retrieve the vertex/fragment source code from filepath
    std::string vertexCode;
    std::string fragmentCode;
    std::string tessControlCode;
    std::string tessEvaluationCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    std::ifstream tessControlFile;
    std::ifstream tessEvaluationFile;
    const char* vShaderCode;
    const char* fShaderCode;
    const char* tcShaderCode;
    const char* teShaderCode;
    // ensure ifstream objects can throw exceptions:
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    tessControlFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    tessEvaluationFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        // open files
        vShaderFile.open(Config::ProjectRootDir + "/" + vertexPath);
        fShaderFile.open(Config::ProjectRootDir + "/" + fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        // read file's buffer contents into streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        // close file handlers
        vShaderFile.close();
        fShaderFile.close();
        // convert stream into string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
        // if tessellation control shader path is present, also load it
        if (tessControlPath != "") {
            tessControlFile.open(Config::ProjectRootDir + "/" + tessControlPath);
            std::stringstream tcShaderStream;
            tcShaderStream << tessControlFile.rdbuf();
            tessControlFile.close();
            tessControlCode = tcShaderStream.str();
        }
        // if tessellation evaluation shader path is present, also load it
        if (tessEvaluationPath != "") {
            tessEvaluationFile.open(Config::ProjectRootDir + "/" + tessEvaluationPath);
            std::stringstream teShaderStream;
            teShaderStream << tessEvaluationFile.rdbuf();
            tessEvaluationFile.close();
            tessEvaluationCode = teShaderStream.str();
        }
    } catch (std::ifstream::failure& e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << strerror(errno)
                  << std::endl;
    }

    vShaderCode = vertexCode.c_str();
    fShaderCode = fragmentCode.c_str();
    if (tessControlPath != "") tcShaderCode = tessControlCode.c_str();
    if (tessEvaluationPath != "") teShaderCode = tessEvaluationCode.c_str();
    // 2. compile shaders
    unsigned int vertex, fragment, tessControl, tessEvaluation;
    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(vertex, 1, &vShaderCode, NULL);

    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");
    // fragment ShaderProgram
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");
    // if geometry shader is given, compile geometry shader
    if (tessControlPath != "") {
        tessControl = glCreateShader(GL_TESS_CONTROL_SHADER);
        glShaderSource(tessControl, 1, &tcShaderCode, NULL);
        glCompileShader(tessControl);
        checkCompileErrors(tessControl, "TESS_CONTROL");
    }

    if (tessEvaluationPath != "") {
        tessEvaluation = glCreateShader(GL_TESS_EVALUATION_SHADER);
        glShaderSource(tessEvaluation, 1, &teShaderCode, NULL);
        glCompileShader(tessEvaluation);
        checkCompileErrors(tessEvaluation, "TESS_EVALUATION");
    }
    // vertex and fragment shaders are already compiled above, no need to compile again
    // shader Program
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    if (tessControlPath != "") glAttachShader(ID, tessControl);
    if (tessEvaluationPath != "") glAttachShader(ID, tessEvaluation);
    glLinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");

    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    if (tessControlPath != "") glDeleteShader(tessControl);
    if (tessEvaluationPath != "") glDeleteShader(tessEvaluation);

    cacheActiveUniforms();  // Cache uniform locations after linking the program

    std::cout << "ShaderProgram created with ID: " << ID << std::endl;
}
// activate the shader
// ------------------------------------------------------------------------

void ShaderProgram::use() {
    glUseProgram(ID);
}
// utility uniform functions
// ------------------------------------------------------------------------
void ShaderProgram::setBool(const std::string& name, bool value) {
    GLint location = getUniformLocation(name.c_str());
    glUniform1i(location, (int)value);
}
// ------------------------------------------------------------------------
void ShaderProgram::setInt(const std::string& name, int value) {
    GLint location = getUniformLocation(name.c_str());
    glUniform1i(location, value);
}
// ------------------------------------------------------------------------
void ShaderProgram::setFloat(const std::string& name, float value) {
    GLint location = getUniformLocation(name.c_str());
    glUniform1f(location, value);
}
// ------------------------------------------------------------------------
void ShaderProgram::setVec2(const std::string& name, const glm::vec2& value) {
    GLint location = getUniformLocation(name.c_str());
    glUniform2fv(location, 1, &value[0]);
}
void ShaderProgram::setVec2(const std::string& name, float x, float y) {
    GLint location = getUniformLocation(name.c_str());
    glUniform2f(location, x, y);
}
// ------------------------------------------------------------------------
void ShaderProgram::setVec3(const std::string& name, const glm::vec3& value) {
    GLint location = getUniformLocation(name.c_str());
    glUniform3fv(location, 1, &value[0]);
}
void ShaderProgram::setVec3(const std::string& name, float x, float y, float z) {
    GLint location = getUniformLocation(name.c_str());
    glUniform3f(location, x, y, z);
}
// ------------------------------------------------------------------------
void ShaderProgram::setVec4(const std::string& name, const glm::vec4& value) {
    GLint location = getUniformLocation(name.c_str());
    glUniform4fv(location, 1, &value[0]);
}
void ShaderProgram::setVec4(const std::string& name, float x, float y, float z, float w) {
    GLint location = getUniformLocation(name.c_str());
    glUniform4f(location, x, y, z, w);
}
// ------------------------------------------------------------------------
void ShaderProgram::setMat2(const std::string& name, const glm::mat2& mat) {
    GLint location = getUniformLocation(name.c_str());
    glUniformMatrix2fv(location, 1, GL_FALSE, &mat[0][0]);
}
// ------------------------------------------------------------------------
void ShaderProgram::setMat3(const std::string& name, const glm::mat3& mat) {
    GLint location = getUniformLocation(name.c_str());
    glUniformMatrix3fv(location, 1, GL_FALSE, &mat[0][0]);
}
// ------------------------------------------------------------------------
void ShaderProgram::setMat4(const std::string& name, const glm::mat4& mat) {
    GLint location = getUniformLocation(name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, &mat[0][0]);
}

// ------------------------------------------------------------------------
void ShaderProgram::setInt(const char* name, int value) {
    GLint location = getUniformLocation(name);
    glUniform1i(location, value);
}
void ShaderProgram::setFloat(const char* name, float value) {
    GLint location = getUniformLocation(name);
    glUniform1f(location, value);
}
void ShaderProgram::setVec2(const char* name, const glm::vec2& value) {
    GLint location = getUniformLocation(name);
    glUniform2fv(location, 1, &value[0]);
}
void ShaderProgram::setVec3(const char* name, const glm::vec3& value) {
    GLint location = getUniformLocation(name);
    glUniform3fv(location, 1, &value[0]);
}
void ShaderProgram::setVec4(const char* name, const glm::vec4& value) {
    GLint location = getUniformLocation(name);
    glUniform4fv(location, 1, &value[0]);
}
void ShaderProgram::setMat3(const char* name, const glm::mat3& mat) {
    GLint location = getUniformLocation(name);
    glUniformMatrix3fv(location, 1, GL_FALSE, &mat[0][0]);
}
void ShaderProgram::setMat4(const char* name, const glm::mat4& mat) {
    GLint location = getUniformLocation(name);
    glUniformMatrix4fv(location, 1, GL_FALSE, &mat[0][0]);
}

GLint ShaderProgram::getUniformLocation(const char* name) {
    // Check if the location is already cached
    auto it = uniformLocationCache.find(name);
    if (it != uniformLocationCache.end()) {
        return it->second;
    }

    // If not cached, query OpenGL for the location
    GLint location = glGetUniformLocation(ID, name);

    // Cache the location for future use
    uniformLocationCache[name] = location;
    return location;
}

void ShaderProgram::cacheActiveUniforms() {
    GLint numUniforms = 0;
    glGetProgramiv(ID, GL_ACTIVE_UNIFORMS, &numUniforms);

    for (GLint i = 0; i < numUniforms; i++) {
        char glName[256];
        GLsizei length;
        GLint size;
        GLenum type;
        glGetActiveUniform(ID, i, sizeof(glName), &length, &size, &type, glName);

        UniformName name = fromCString(glName);
        if (name != UniformName::Count) {
            uniformLocationCache[glName] = glGetUniformLocation(ID, glName);
        }
    }
}

void ShaderProgram::checkCompileErrors(GLuint shader, std::string type) {
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                      << infoLog << "\n -- --------------------------------------------------- -- "
                      << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
                      << infoLog << "\n -- --------------------------------------------------- -- "
                      << std::endl;
        }
    }
}