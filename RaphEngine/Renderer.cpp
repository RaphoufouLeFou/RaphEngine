#include "pch.h"
#include "include/Renderer.h"
#include "include/RaphEngine.h"
#include "include/ImageLoader.h"
#include <iostream>


#include <ft2build.h>
#include FT_FREETYPE_H
#include <SDL_ttf.h>
#include <map>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>


#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <GL/GL.h>

#define PI 3.14159265359
#define DEG2RAD(x) (x * PI / 180)

glm::mat4 ViewMatrix = glm::mat4(1.0f);
glm::mat4 ProjectionMatrix = glm::mat4(1.0f);

GLFWwindow* window;
const int* Renderer::ResX;
const int* Renderer::ResY;

Shader* textShader;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions
	glViewport(0, 0, width, height);
}

void SetHints() {
	glfwWindowHint(GLFW_SAMPLES, 8); // 8x antialiasing
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE); // Maximizing window

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL 
}

glm::vec3 Vector3ToVec3(Vector3 vec) {
	return glm::vec3(vec.x, vec.y, vec.z);
}

glm::vec2 Vector2ToVec2(Vector2 vec) {
	return glm::vec2(vec.x, vec.y);
}

void CalculateMat() {

	ProjectionMatrix = glm::perspective(glm::radians(RaphEngine::camera->fov), (float)(*Renderer::ResX) / (float)(*Renderer::ResY), 0.1f, 1000.0f);
	float coef = 70;
	float X = (float)(*Renderer::ResX) / 2 / coef;
	float Y = (float)(*Renderer::ResY) / 2 / coef;
	// ProjectionMatrix = glm::ortho(-X, X, -Y, Y, 0.3f, 300.00f);
	// Camera matrix


	glm::vec3 direction(
		cos(DEG2RAD(RaphEngine::camera->transform->rotation.y - 90)) * cos(DEG2RAD(RaphEngine::camera->transform->rotation.x)),
		sin(DEG2RAD(RaphEngine::camera->transform->rotation.x)),
		sin(DEG2RAD(RaphEngine::camera->transform->rotation.y - 90)) * cos(DEG2RAD(RaphEngine::camera->transform->rotation.x))
	);

	glm::vec3 right = glm::vec3(
		cos(DEG2RAD(RaphEngine::camera->transform->rotation.y - 90) - PI / 2.0f),
		0,
		sin(DEG2RAD(RaphEngine::camera->transform->rotation.y - 90) - PI / 2.0f)
	);

	glm::vec3 up = glm::cross(right, direction);

	ViewMatrix = glm::lookAt(
		Vector3ToVec3(RaphEngine::camera->transform->position),
		Vector3ToVec3(RaphEngine::camera->transform->position) + direction, // and looks here : at the same position, plus "direction"
		up                  // Head is up (set to 0,-1,0 to look upside-down)
	);
}

void Renderer::Init(bool fullScreen) {
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		exit(EXIT_FAILURE);
	}

	SetHints();

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();

	ResX = &glfwGetVideoMode(monitor)->width;
	ResY = &glfwGetVideoMode(monitor)->height;

	window = glfwCreateWindow(*ResX, *ResY, RaphEngine::windowTitle, fullScreen ? monitor : NULL, NULL);

	if (!window) {
		std::cerr << "Failed to create window" << std::endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window); // Initialize GLEW
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	

	glewExperimental = true; // Needed in core profile
	if (glewInit() != GLEW_OK) {
		std::cerr << "Failed to initialize GLEW" << std::endl;
		exit(EXIT_FAILURE);
		return;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	// Hide the mouse and enable unlimited movement
	// glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set the mouse at the center of the screen
	glfwPollEvents();
	//glfwSetCursorPos(window, *ResX / 2, *ResY / 2);

	// Dark blue background
	glClearColor(0.36f, 0.74f, 0.89f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it is closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_DEBUG_OUTPUT);
	Text::InitTextRendering();
}

std::vector<glm::vec3> Vector3ToVec3(std::vector<Vector3> vec) {
	std::vector<glm::vec3> result;
	for (Vector3 v : vec) {
		result.push_back(glm::vec3(v.x, v.y, v.z));
	}
	return result;
}

std::vector<glm::vec2> Vector2ToVec2(std::vector<Vector2> vec) {
	std::vector<glm::vec2> result;
	for (Vector2 v : vec) {
		result.push_back(glm::vec2(v.x, v.y));
	}
	return result;
}

void UI::RenderImage(Shader& shader, GLuint Texture, int x, int y, int sizeX, int sizeY, int zIndex) {

	float relativeX = (float)x / *Renderer::ResX;
	float relativeY = (float)y / *Renderer::ResY;
	float relativeSizeX = (float)sizeX / *Renderer::ResX;
	float relativeSizeY = (float)sizeY / *Renderer::ResY;

	float vertices[] = {
		relativeX                , relativeY + relativeSizeY, zIndex, 01, // left
		relativeX                , relativeY                , zIndex, 00, // right
		relativeX + relativeSizeX, relativeY                , zIndex, 10, // top

		relativeX                , relativeY + relativeSizeY, zIndex, 01, // left
		relativeX + relativeSizeX, relativeY                , zIndex, 10, // top
		relativeX + relativeSizeX, relativeY + relativeSizeY, zIndex, 11  // right
	};

	glBindTexture(GL_TEXTURE_2D, Texture);
	shader.setInt("TextureSampler", 0);

	unsigned int VBO, VAO;

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	glUseProgram(shader.ID);
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(shader.ID);
	glDeleteTextures(1, &Texture);

}

void RenderGameObject(GameObject * go) {
	int err = 0;

	if (go->mesh->vertices.size() != go->mesh->uvs.size() || go->mesh->vertices.size() != go->mesh->normals.size()) {
		std::cerr << "Vertices, UVs and Normals must have the same size" << std::endl;
		return;
	}

	if (go->mesh->vertices.size() == 0)
		return;


	go->mesh->shader->use();

	glm::mat4 ModelMatrix = glm::mat4(1.0);
	glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

	go->mesh->shader ->setMat4("MVP", MVP);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, go->mesh->texture);

	go->mesh->shader->setInt("myTextureSampler", 0);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint uvbuffer;
	GLuint vertexbuffer;
	GLuint normalbuffer;

	std::vector<glm::vec3> indexed_vertices = Vector3ToVec3(go->mesh->vertices);
	std::vector<glm::vec2> indexed_uvs = Vector2ToVec2(go->mesh->uvs);
	std::vector<glm::vec3> indexed_normals = Vector3ToVec3(go->mesh->normals);

	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW);

	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3), &indexed_normals[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

	Vector3 lightPos(0, 40, 0);

	go->mesh->shader->setVec3("lightPos", lightPos);
	go->mesh->shader->setVec3("ObjectPosition", go->transform->position);
	go->mesh->shader->setVec3("ObjectRotation", go->transform->rotation);
	go->mesh->shader->setVec3("ObjectScale", go->transform->scale);
	go->mesh->shader->setBool("isTerrain", false);

	glDrawArrays(GL_TRIANGLES, 0, (int)indexed_vertices.size());

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteBuffers(1, &normalbuffer);

	indexed_uvs.clear();
	indexed_vertices.clear();
	indexed_normals.clear();
}

void RenderObjects() {
	CalculateMat();
	for (GameObject* go : GameObject::SpawnedGameObjects) {
		if(go->activeSelf && go->mesh)
			RenderGameObject(go);
	}
}

bool Renderer::IsKeyPressed(int key) {
	return glfwGetKey(window, key) == GLFW_PRESS;
}

std::map<GLchar, Character> Characters;
unsigned int VAO, VBO;

void Text::InitTextRendering() {
	textShader = new Shader("shader/text.vs", "shader/text.fs");
	glm::mat4 projection = glm::ortho(0.0f, (float)(*Renderer::ResX), 0.0f, (float)(*Renderer::ResY));
	textShader->use();
	glUniformMatrix4fv(glGetUniformLocation(textShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	// FreeType
	FT_Library ft;
	// All functions return a value different than 0 whenever an error occurred
	if (FT_Init_FreeType(&ft))
	{
		std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
		return;
	}

	std::string font_name = "D:/DOCUMENTS/GitHub/GLMC/GLMC/fonts/Pixellari.ttf";
	if (font_name.empty())
	{
		std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
		return;
	}

	// load font as face
	FT_Face face;
	if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
		return;
	}
	else {
		// set size to load glyphs as
		FT_Set_Pixel_Sizes(face, 0, 48);

		// disable byte-alignment restriction
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		// load first 128 characters of ASCII set
		for (unsigned char c = 0; c < 128; c++)
		{
			// Load character glyph 
			if (FT_Load_Char(face, c, FT_LOAD_RENDER))
			{
				std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
				continue;
			}
			// generate texture
			unsigned int texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RED,
				face->glyph->bitmap.width,
				face->glyph->bitmap.rows,
				0,
				GL_RED,
				GL_UNSIGNED_BYTE,
				face->glyph->bitmap.buffer
			);
			// set texture options
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			// now store character for later use
			Character character = {
				texture,
				glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
				glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
				static_cast<unsigned int>(face->glyph->advance.x)
			};
			Characters.insert(std::pair<char, Character>(c, character));

		}
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// destroy FreeType once we're finished
	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

}

void Text::RenderText(std::string text, float x, float y, float scale, glm::vec3 color)
{
	GLuint VertexArrayID;
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// activate corresponding render state	
	textShader->use();
	glUniform3f(glGetUniformLocation(textShader->ID, "textColor"), color.x, color.y, color.z);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(VAO);
	// iterate through all characters
	std::string::const_iterator c;

	float CurrentX = x;

	for (c = text.begin(); c != text.end(); c++)
	{
		if (*c == '\n')
		{
			y -= 20;
			CurrentX = x;
			continue;
		}
		Character ch = Characters[*c];

		float xpos = CurrentX + ch.Bearing.x * scale;
		float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

		float w = ch.Size.x * scale;
		float h = ch.Size.y * scale;
		// update VBO for each character
		float vertices[6][4] = {
			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos,     ypos,       0.0f, 1.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },

			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },
			{ xpos + w, ypos + h,   1.0f, 0.0f }
		};
		// render glyph texture over quad
		glBindTexture(GL_TEXTURE_2D, ch.TextureID);
		// update content of VBO memory
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		CurrentX += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_BLEND);


	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	glDisableVertexAttribArray(0);

}

bool Renderer::RenderFrame() {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Shader shader = Shader("shader/imagesVS.glsl", "shader/imagesFS.glsl");
	//GLuint Texture = ImageLoader::LoadImageGL("Textures/Logo.bmp");
	//UI::RenderImage(shader, Texture, -*ResX, -*ResY, 2 ** ResX, 2 ** ResY, 0);

	RenderObjects();

	// text rendering
	Text::RenderText("alpha dev build : vA0.000.1", 2, 2, 0.4f, glm::vec3(0, 0, 0));

	glfwSwapBuffers(window);

	glfwPollEvents();

	return glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0;
}


void Renderer::UpdateLogoGL(const char* newLogoPath) {
	GLFWimage* image = ImageLoader::LoadImageDataGL(newLogoPath);
	if(!image) {
		std::cerr << "Failed to load Logo" << std::endl;
		return;
	}
	glfwSetWindowIcon(window, 1, image);
}
