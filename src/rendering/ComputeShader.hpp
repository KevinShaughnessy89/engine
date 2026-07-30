#pragma once
#include "core/PrecompiledHeader.hpp"

#include "ShaderProgram.hpp"

class ComputeShader : public ShaderProgram {
   public:
    ComputeShader(const std::string& path) {
        std::string computeCode;
        std::ifstream cShaderFile;
        // ensure ifstream objects can throw exceptions:
        cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            // open files
            cShaderFile.open(Config::ProjectRootDir + "/" + path);
            std::stringstream cShaderStream;
            // read file's buffer contents into streams
            cShaderStream << cShaderFile.rdbuf();
            // close file handlers
            cShaderFile.close();
            // convert stream into string
            computeCode = cShaderStream.str();
            // if geometry shader path is present, also load a geometry shader

        } catch (std::ifstream::failure& e) {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what()
                      << strerror(errno) << std::endl;
        }

        const char* cShaderCode = computeCode.c_str();

        GLuint compute = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(compute, 1, &cShaderCode, NULL);

        glCompileShader(compute);

        GLint success;
        glGetShaderiv(compute, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLint logLength = 0;
            glGetShaderiv(compute, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<GLchar> infoLog(logLength);
            glGetShaderInfoLog(compute, logLength, NULL, infoLog.data());
            std::cerr << "ERROR::COMPUTE_SHADER_COMPILATION_FAILED\n"
                      << infoLog.data() << std::endl;
        }

        ID = glCreateProgram();
        glAttachShader(ID, compute);
        glLinkProgram(ID);

        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success) {
            GLint logLength = 0;
            glGetProgramiv(ID, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<GLchar> infoLog(logLength);
            glGetProgramInfoLog(ID, logLength, NULL, infoLog.data());
            std::cerr << "ERROR::PROGRAM_LINKING_FAILED\n" << infoLog.data() << std::endl;
        }

        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(compute);

        std::cout << "Compute shader created with ID: " << ID << std::endl;
    }

    // Dispatch compute shader work groups
    void dispatch(GLuint groupsX, GLuint groupsY, GLuint groupsZ = 1) {
        glDispatchCompute(groupsX, groupsY, groupsZ);

        // Memory barrier to ensure compute writes are visible to subsequent operations,
        // including sampler2D reads of the written textures
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    // Get compute work group size (useful for calculating dispatch groups)
    void getWorkGroupSize(GLint& sizeX, GLint& sizeY, GLint& sizeZ) {
        glGetProgramiv(ID, GL_COMPUTE_WORK_GROUP_SIZE, &sizeX);
        // OpenGL returns all three dimensions in a single call, but this varies by implementation
        // Safer to query each dimension separately:
        glGetProgramiv(ID, GL_COMPUTE_WORK_GROUP_SIZE + 0, &sizeX);
        glGetProgramiv(ID, GL_COMPUTE_WORK_GROUP_SIZE + 1, &sizeY);
        glGetProgramiv(ID, GL_COMPUTE_WORK_GROUP_SIZE + 2, &sizeZ);
    }
};
