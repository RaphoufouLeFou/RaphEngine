#pragma once

#include "RaphEngine.h"
#include <string>

class RAPHENGINE_API Image {
public:
	static void InitImageRendering();
	static void RenderImage(std::string path, int x, int y, int sizeX, int sizeY, int zIndex);
	static void RenderImage(unsigned int texture, int x, int y, int sizeX, int sizeY, int zIndex);

};