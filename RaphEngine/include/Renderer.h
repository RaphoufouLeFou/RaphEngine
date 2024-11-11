#pragma once

#include "RaphEngine.h"
#include "Shader.h"

class Renderer {
public:
	static const int* ResX;
	static const int* ResY;
	static void Init(bool fullScreen);
	static bool IsKeyPressed(KeyCode key);
	static void StartFrameRender();
	static bool RenderFrame();
	static void UpdateLogoGL(const char* newLogoPath);
};


struct Character {
	unsigned int TextureID; // ID handle of the glyph texture
	glm::ivec2   Size;      // Size of glyph
	glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
	unsigned int Advance;   // Horizontal offset to advance to next glyph
};

