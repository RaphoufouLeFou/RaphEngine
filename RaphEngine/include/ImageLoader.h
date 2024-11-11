#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

class ImageLoader
{
public:
	static GLFWimage* LoadImageDataGL(const char * path);
	static GLuint LoadImageGL(const char * path, bool smooth);
};

