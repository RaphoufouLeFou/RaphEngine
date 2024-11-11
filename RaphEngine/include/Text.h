#pragma once

#include "RaphEngine.h"
#include <string>

class RAPHENGINE_API Text {
public:
	static void InitTextRendering();
	static void RenderText(std::string text, float x, float y, float scale, Vector3 color);
};