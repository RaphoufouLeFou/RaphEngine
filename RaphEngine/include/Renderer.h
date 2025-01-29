#pragma once

#include "RaphEngine.h"
#include "Shader.h"
#include <glm.hpp>

class Renderer {
public:
	static int* ResX;
	static int* ResY;
	static void Init(bool fullScreen, std::string font_name);
	static bool IsKeyPressed(KeyCode key);
	static void StartFrameRender();
	static bool RenderFrame();
	static GLFWwindow* GetWindow();
	static void UpdateLogoGL(const char* newLogoPath);
	static glm::mat4 GetProjectionMatrix();
	static glm::mat4 GetViewMatrix();
};


struct Character {
	unsigned int TextureID; // ID handle of the glyph texture
	glm::ivec2   Size;      // Size of glyph
	glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
	unsigned int Advance;   // Horizontal offset to advance to next glyph
};

