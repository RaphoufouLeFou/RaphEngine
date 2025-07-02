#pragma once

#ifdef _WIN32
#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <GL/GL.h>
#include <glm.hpp>
#else
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <glm/glm.hpp>
#endif

#include "Vector.h"
#include "shaders.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
    unsigned int ID;
    Shader(const char* vShaderCode, const char* fShaderCode, const char* gShaderCode = nullptr);
    void use();
    void setBool(const char* name, bool value) const;
    void setInt(const char* name, int value) const;
    void setIntArray(const char* name, int count, int *value) const;
    void setFloat(const char* name, float value) const;
    void setFloatArray(const char* name, int count, float *value) const;
    void setVec2(const char* name, const Vector2 value) const;
    void setVec2(const char* name, float x, float y) const;
    void setVec2Array(const char* name, int count, const Vector2* value) const;
    void setVec3(const char* name, const Vector3 value) const;
    void setVec3Array(const char* name, int count, const Vector3 *value) const;
    void setVec3(const char* name, float x, float y, float z) const;
    void setVec4(const char* name, const Vector4& value) const;
    void setVec4(const char* name, float x, float y, float z, float w);
    void setMat2(const char* name, const Matrix2& mat) const;
    void setMat3(const char* name, const Matrix3& mat) const;
    void setMat4(const char* name, const Matrix4& mat) const;
    static std::vector<Shader*> LoadedShaders;

private:
    void checkCompileErrors(unsigned int shader, std::string type);
};