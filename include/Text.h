#pragma once

#include "RaphEngine.h"
#include <string>

class RAPHENGINE_API Text {
public:
	static void InitTextRendering(std::string font_name);
	static void RenderText(const char* text, float x, float y, float scale, Vector3 color);
};