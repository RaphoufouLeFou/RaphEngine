#pragma once

#include "RaphEngine.h"
#include "Shader.h"

class Renderer {
public:
	static const int* ResX;
	static const int* ResY;
	static void Init(bool fullScreen);
	static bool IsKeyPressed(int key);
	static bool RenderFrame();
	static void UpdateLogoGL(const char* newLogoPath);
};

class UI {
public:
	static void RenderImage(Shader& shader, GLuint Texture, int x, int y, int sizeX, int sizeY, int zIndex);
};

struct Character {
	unsigned int TextureID; // ID handle of the glyph texture
	glm::ivec2   Size;      // Size of glyph
	glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
	unsigned int Advance;   // Horizontal offset to advance to next glyph
};


class Text {
public:
	static void InitTextRendering();
	static void RenderText(std::string text, float x, float y, float scale, glm::vec3 color);
};