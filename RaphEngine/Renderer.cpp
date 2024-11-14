#include "pch.h"
#include "include/Renderer.h"
#include "include/RaphEngine.h"
#include "include/ImageLoader.h"
#include "include/Text.h"
#include "include/Images.h"
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
glm::mat4 ModelMatrix = glm::mat4(1.0f);
glm::mat4 MVP = glm::mat4(1.0f);

GLFWwindow* window;
int* Renderer::ResX;
int* Renderer::ResY;

Shader* textShader;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions
	glViewport(0, 0, width, height);
	*Renderer::ResX = width;
	*Renderer::ResY = height;
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
	float coef = 60;
	float X = (float)(*Renderer::ResX) / 2 / coef;
	float Y = (float)(*Renderer::ResY) / 2 / coef;
	// ProjectionMatrix = glm::ortho(-X, X, -Y, Y, 0.3f, 300.00f);
	// Camera matrix

	glm::vec3 direction(
		cos(DEG2RAD(RaphEngine::camera->transform->rotation.y - 90)) * cos(DEG2RAD(RaphEngine::camera->transform->rotation.x)),
		sin(DEG2RAD(RaphEngine::camera->transform->rotation.x)),
		sin(DEG2RAD(RaphEngine::camera->transform->rotation.y - 90)) * cos(DEG2RAD(RaphEngine::camera->transform->rotation.x))
	);

	// printf("Direction: %f %f %f\n", direction.x, direction.y, direction.z);

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


	glm::mat4 Model = glm::mat4(1.0);
	MVP = ProjectionMatrix * ViewMatrix * Model;
}

const int factor = 8;
const unsigned int SHADOW_WIDTH = 1024 * factor, SHADOW_HEIGHT = 1024 * factor;
unsigned int depthMapFBO;
// create depth texture
unsigned int depthMap;

Shader *shader;
Shader *simpleDepthShader;
Shader *debugDepthQuad;


void Renderer::Init(bool fullScreen) {
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		exit(EXIT_FAILURE);
	}

	SetHints();

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();

	ResX = (int *) &glfwGetVideoMode(monitor)->width;
	ResY = (int *) &glfwGetVideoMode(monitor)->height;

	window = glfwCreateWindow(*ResX, *ResY, RaphEngine::windowTitle, fullScreen ? monitor : NULL, NULL);

	if (!window) {
		std::cerr << "Failed to create window" << std::endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}


	glfwGetWindowSize(window, Renderer::ResX, Renderer::ResY);
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
	glfwSwapInterval(0);

	shader = new Shader(shadow_mappingVS_shader, shadow_mappingFS_shader);
	simpleDepthShader = new Shader(shadow_mapping_depthVS_shader, shadow_mapping_depthFS_shader);
	debugDepthQuad = new Shader(debug_quad_VS_shader, debug_quad_depth_FS_shader);


	glGenFramebuffers(1, &depthMapFBO);
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	// attach depth texture as FBO's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	shader->use();
	shader->setInt("diffuseTexture", 0);
	shader->setInt("shadowMap", 1);
	debugDepthQuad->use();
	debugDepthQuad->setInt("depthMap", 0);
}

double Time::GetTime() {
	return glfwGetTime() * 1000;
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

Shader* ImageShader;

void Image::InitImageRendering() {
	ImageShader = new Shader(imagesVS_shader, imagesFS_shader);
}

void Image::RenderImage(std::string path, int x, int y, int sizeX, int sizeY, int zIndex) {

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

	static std::map<std::string, GLuint> Textures;

	if (Textures.find(path) == Textures.end()) {
		GLuint Texture = ImageLoader::LoadImageGL(path.c_str(), false);
		Textures.insert(std::pair<std::string, GLuint>(path, Texture));
	}

	GLuint Texture = Textures[path];
	glBindTexture(GL_TEXTURE_2D, Texture);
	ImageShader->setInt("TextureSampler", 0);

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

	glUseProgram(ImageShader->ID);
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(ImageShader->ID);
	glDeleteTextures(1, &Texture);
}

void Image::RenderImage(GLuint texture, int x, int y, int sizeX, int sizeY, int zIndex) {

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

	glBindTexture(GL_TEXTURE_2D, texture);
	ImageShader->setInt("TextureSampler", 0);

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

	glUseProgram(ImageShader->ID);
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(ImageShader->ID);
}

void Mesh::GenerateBuffers() {

	if (generatedBuffers)
	{
		glDeleteBuffers(1, &vertexbuffer);
		glDeleteBuffers(1, &uvbuffer);
		glDeleteBuffers(1, &normalbuffer);
		glDeleteVertexArrays(1, &vao);
	}

	

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(Vector3), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(Vector2), uvs, GL_STATIC_DRAW);

	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(Vector3), normals, GL_STATIC_DRAW);

	generatedBuffers = true;
}

Vector3 ApplyRotation(Vector3 vec, float rotation) {
	Vector3 result;
	result.x = vec.x * cos(rotation) - vec.z * sin(rotation);
	result.y = vec.y;
	result.z = vec.x * sin(rotation) + vec.z * cos(rotation);
	return result;
}

glm::mat4 lightSpaceMatrix;

void RenderGameObject(GameObject * go, Shader* sh, bool shadowRender) {
	int err = 0;

	if (go->mesh->verticesCount == 0)
		return;
	if (!go->mesh->castShadows && shadowRender)
		return;

	Shader* shaderUse = sh == nullptr ? go->mesh->shader : sh;
	shaderUse->use();

	shaderUse->setMat4("projection", ProjectionMatrix);
	shaderUse->setMat4("view", ViewMatrix);

	ModelMatrix = glm::mat4(1.0f);
	ModelMatrix = glm::translate(ModelMatrix, Vector3ToVec3(go->transform->position));
	ModelMatrix = glm::rotate(ModelMatrix, -(go->transform->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	ModelMatrix = glm::rotate(ModelMatrix, -(go->transform->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)); 
	ModelMatrix = glm::rotate(ModelMatrix, -(go->transform->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	ModelMatrix = glm::scale(ModelMatrix, Vector3ToVec3(go->transform->scale));

	shaderUse->setMat4("model", ModelMatrix);
	// shaderUse->setMat4("MVP", MVP);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, go->mesh->texture);

	shaderUse->setInt("myTextureSampler", 0);


	GLuint vao = go->mesh->vao;

	GLuint uvbuffer = go->mesh->uvbuffer;
	GLuint vertexbuffer = go->mesh->vertexbuffer;
	GLuint normalbuffer = go->mesh->normalbuffer;

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

	int lightCount = 1;

	Vector3 *LightPoses = new Vector3[lightCount];
	LightPoses[0] = /*RaphEngine::camera->transform->position;*/ Vector3(-2.0f, 35.0f, -1.0f);

	Vector3 *lightColor = new Vector3[lightCount];
	lightColor[0] = Vector3(1, 1, 1);

	Vector3 *lightSettings = new Vector3[lightCount];
	lightSettings[0] = Vector3(1, 10, 0);
	

	shaderUse->setVec3Array("lightColor", lightCount, lightColor);
	shaderUse->setVec4("lightPos", glm::vec4(1, 1, 0, 0.0f));
	//shaderUse->setVec3Array("lightSettings", lightCount, lightSettings);


	shaderUse->setInt("lightCount", lightCount);

	Vector3 rotationVect = go->transform->rotation;
	GameObject* parent = go->parent;
	/*
	while (parent != nullptr) {
		rotationVect += parent->transform->rotation;
		parent = parent->parent;
	}*/

	glm::mat3 rotation = glm::mat3(1.0);
	rotation[0] = glm::vec3(cos((rotationVect.y)), 0, sin((rotationVect.y)));
	rotation[1] = glm::vec3(0, 1, 0);
	rotation[2] = glm::vec3(-sin((rotationVect.y)), 0, cos((rotationVect.y)));
	
	Vector3 position = go->transform->position;
	parent = go->parent;
	/*
	while (parent != nullptr) {
		Vector3 newPos = ApplyRotation(parent->transform->position, parent->transform->rotation.y);
		position += Vector3(newPos.x, newPos.y, newPos.z);
		parent = parent->parent;
	}
	*/
	shaderUse->setVec3("ObjectPosition", position);


	shaderUse->setMat3("ObjectRotation", rotation);
	shaderUse->setVec3("ObjectScale", go->transform->scale);
	shaderUse->setBool("isTerrain", false);

	// -- shadow --
	// shaderUse->setVec3("viewPos", RaphEngine::camera->transform->position);
	// shaderUse->setVec3("lightPos", (-2.0f, 4.0f, -1.0f));
	shaderUse->setMat4("lightSpaceMatrix", lightSpaceMatrix);
	// -- shadow --

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, go->mesh->texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);

	glDrawArrays(GL_TRIANGLES, 0, go->mesh->verticesCount);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	delete[] LightPoses;
	delete[] lightColor;
	delete[] lightSettings;
}

void RenderObjects(Shader* sh, bool shadowRender) {
	CalculateMat();
	for (GameObject* go : GameObject::SpawnedGameObjects) {
		if(go->activeSelf && go->mesh)
			RenderGameObject(go, sh, shadowRender);
	}
}

bool Renderer::IsKeyPressed(KeyCode key) {
	return glfwGetKey(window, (int)key) == GLFW_PRESS;
}

std::map<GLchar, Character> Characters;
unsigned int VAO, VBO;

void Text::InitTextRendering() {
	textShader = new Shader(textVS_shader, textFS_shader);
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

	std::string font_name = "Assets/Fonts/Pixellari.ttf";
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

void Text::RenderText(const char* text, float x, float y, float scale, Vector3 color)
{
	static GLuint VertexArrayID;
	glDeleteVertexArrays(1, &VertexArrayID);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// activate corresponding render state	
	textShader->use();
	glUniform3f(glGetUniformLocation(textShader->ID, "textColor"), color.x, color.y, color.z);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(VAO);
	// iterate through all characters

	float CurrentX = x;
	int len = strlen(text);
	for (int i = 0; i < len; i++)
	{
		char c = text[i];
		if (c == '\n')
		{
			y -= 20;
			CurrentX = x;
			continue;
		}
		Character ch = Characters[c];

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

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void Renderer::StartFrameRender() {

	// 1. render depth of scene to texture (from light's perspective)
		// --------------------------------------------------------------
	glm::mat4 lightProjection, lightView;
	
	float near_plane = 1.f, far_plane = 7.5f;

	///lightProjection = glm::perspective(glm::radians(165.f), (GLfloat)SHADOW_WIDTH / (GLfloat)SHADOW_HEIGHT, near_plane, far_plane); // note that if you use a perspective projection matrix you'll have to change the light position as the current light position isn't enough to reflect the whole scene
	lightProjection = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, near_plane, far_plane);
	Vector3 pos = RaphEngine::camera->transform->position;
	if (RaphEngine::Player != nullptr)
	{

		pos = RaphEngine::Player->transform->position + Vector3(
			5 * sin(RaphEngine::Player->transform->rotation.y),
			3,
			-5 * cos(RaphEngine::Player->transform->rotation.y)
		);
	}
	glm::vec3 lightPos = Vector3ToVec3(pos);// (-2.0f, 35.0f, -1.0f);
	//lightPos.y += 2;

	Vector3 rotation(-150, 90, 0);
	glm::vec3 direction(
		cos(DEG2RAD(rotation.y - 90)) * cos(DEG2RAD(rotation.x)),
		sin(DEG2RAD(rotation.x)),
		sin(DEG2RAD(rotation.y - 90)) * cos(DEG2RAD(rotation.x))
	);

	glm::vec3 right = glm::vec3(
		cos(DEG2RAD(rotation.y - 90) - PI / 2.0f),
		0,
		sin(DEG2RAD(rotation.y - 90) - PI / 2.0f)
	);

	glm::vec3 up = glm::cross(right, direction);

	lightView = glm::lookAt(
		lightPos,
		lightPos + direction, // and looks here : at the same position, plus "direction"
		up                  // Head is up (set to 0,-1,0 to look upside-down)
	);

	//lightView = glm::lookAt(lightPos, dir, up);
	lightSpaceMatrix = lightProjection * lightView;
	
	// render scene from light's point of view
	simpleDepthShader->use();
	simpleDepthShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glActiveTexture(GL_TEXTURE0);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	//glCullFace(GL_FRONT);
	RenderObjects(simpleDepthShader, true);
	//glCullFace(GL_BACK); // don't forget to reset original culling face
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// reset viewport
	glViewport(0, 0, *ResX, *ResY);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Image::RenderImage(depthMap, 0, 0, 100, 100, 0);
	RenderObjects(shader, false);

	debugDepthQuad->use();
	debugDepthQuad->setFloat("near_plane", near_plane);
	debugDepthQuad->setFloat("far_plane", far_plane);
	glActiveTexture(GL_TEXTURE0);


	//GLuint Texture = ImageLoader::LoadImageGL("Assets/Textures/Logo.bmp", false);
	//glBindTexture(GL_TEXTURE_2D, Texture);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	renderQuad();
}

bool Renderer::RenderFrame() {

	// text rendering
	Text::RenderText("alpha dev build : vA0.000.3", 2, 2, 0.4f, Vector3(0, 0, 0));

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
