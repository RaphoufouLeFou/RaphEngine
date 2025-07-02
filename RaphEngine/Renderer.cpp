#include "pch.h"
#include "include/Renderer.h"
#include "include/RaphEngine.h"
#include "include/ImageLoader.h"
#include "include/Text.h"
#include "include/Images.h"
#include <iostream>
#include <math.h>
#include <random>


#include <ft2build.h>
#include FT_FREETYPE_H
#include <SDL_ttf.h>
#include <map>
#define GLM_ENABLE_EXPERIMENTAL
#ifdef _WIN32
#include <gtx/quaternion.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <GL/GL.h>

#else

#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>

#endif
#define PI 3.14159265359
#define DEG2RAD(x) (x * PI / 180)

glm::mat4 ViewMatrix = glm::mat4(1.0f);
glm::mat4 ProjectionMatrix = glm::mat4(1.0f);
glm::mat4 ModelMatrix = glm::mat4(1.0f);
glm::mat4 MVP = glm::mat4(1.0f);

GLFWwindow* window;
int* Renderer::ResX;
int* Renderer::ResY;
std::vector<std::string> Renderer::skyboxTextures;

Shader* textShader;

std::string fontName;

std::vector<float> shadowCascadeLevels{ 500.0f, 100.0f, 30.0f, 4.0f };
//std::vector<float> shadowCascadeLevels{50.0f, 25.0f, 10.0f, 2.0f };

std::vector<glm::mat4> lightMatricesCache;

int debugLayer = 0;
bool showQuad = false;

glm::vec3 lightDirGlobal = glm::normalize(glm::vec3(-2.5f, -2.5f, -2.0f));

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions
	glViewport(0, 0, width, height);
	*Renderer::ResX = width;
	*Renderer::ResY = height;
	Text::InitTextRendering(fontName);
}

void SetHints() {
	glfwWindowHint(GLFW_SAMPLES, 8); // 8x antialiasing
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE); // Maximizing window

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // We want OpenGL 4.1
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL 
}

void CalculateMat() {

	ProjectionMatrix = glm::perspective(
		glm::radians(RaphEngine::camera->fov),
		(float)(*Renderer::ResX) / (float)(*Renderer::ResY),
		RaphEngine::camera->nearPlane,
		RaphEngine::camera->farPlane
	);
	// float coef = 60;
	// float X = (float)(*Renderer::ResX) / 2 / coef;
	// float Y = (float)(*Renderer::ResY) / 2 / coef;
	// ProjectionMatrix = glm::ortho(-X, X, -Y, Y, 0.3f, 300.00f);
	// Camera matrix

	glm::mat4 RotationMat = glm::toMat4(glm::quat(glm::radians(RaphEngine::camera->transform->GetRotation())));
	glm::vec3 direction = glm::vec3(RotationMat * glm::vec4(0, 1, 0, 1));
	//glm::vec3 right = glm::vec3(RotationMat * glm::vec4(1, 0, 0, 1));
	glm::vec3 up = glm::vec3(RotationMat * glm::vec4(0, 0, 1, 1));
	//glm::vec3 up = glm::cross(right, direction);
	
	glm::vec3 pos = RaphEngine::camera->transform->GetPosition();

	ViewMatrix = glm::lookAt(
		pos,
		pos + direction,
		up
	);

}

const int factor = 4 ;
unsigned int SHADOW_WIDTH = 1024 * factor, SHADOW_HEIGHT = 1024 * factor;
unsigned int depthMapFBO;
// create depth texture
unsigned int depthMap;


Shader *shader;
Shader *skyboxShader;
Shader *simpleDepthShader;
Shader *simpleDepthShaderInstancied;
Shader *debugDepthQuad;
Shader *debugCascadeShader;

#define SAMPLES_COUNT 2
#define SAMPLE_SIZE ((SAMPLES_COUNT * 2 + 1) * (SAMPLES_COUNT * 2 + 1))
Vector2 offsets[SAMPLE_SIZE];


float jitter()
{
    static std::default_random_engine generator;
    static std::uniform_real_distribution<float> distrib(-0.5f, 0.5f);
    return distrib(generator);
}

void Calc_offsets()
{
	for (int i = 0; i < SAMPLE_SIZE; i++)
	{
		float xNoise = jitter();
		float yNoise = jitter();
		offsets[i].x = xNoise;
		offsets[i].y = yNoise;
		//std::cout << "Offset " << i << " is " << offsets[i].x << " " << offsets[i].y << std::endl;
	}
}

glm::mat4 Renderer::GetProjectionMatrix()
{
	return ProjectionMatrix;
}

glm::mat4 Renderer::GetViewMatrix()
{
	return ViewMatrix;
}

unsigned int matricesUBO;

void GenerateShadowBuffer()
{
	glGenFramebuffers(1, &depthMapFBO);

    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D_ARRAY, depthMap);
    glTexImage3D(
        GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, SHADOW_WIDTH, SHADOW_HEIGHT, int(shadowCascadeLevels.size()) + 1,
        0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    constexpr float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, bordercolor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);


    int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete! " << std::endl;
		std::cout << "Status: " << status << std::endl;

    }


	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void SetShadowResolution(Settings::QualitySettings Quality)
{
	unsigned int resolution = 1024;
	switch (Quality)
	{
	case Settings::QualitySettings::Ultra:
		resolution = 4096;
		break;
	case Settings::QualitySettings::High:
		resolution = 2048;
		break;
	case Settings::QualitySettings::Medium:
		resolution = 1024;
		break;
	case Settings::QualitySettings::Low:
		resolution = 512;
		break;
	default:
		break;
	}

	SHADOW_HEIGHT = resolution;
	SHADOW_WIDTH = resolution;
	GenerateShadowBuffer();
}

void Renderer::Init(bool fullScreen, std::string font_name) {
	fontName = font_name;
	Calc_offsets();
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
	// glEnable(GL_CULL_FACE);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_DEBUG_OUTPUT);

	Text::InitTextRendering(fontName);
	Image::InitImageRendering();
	glfwSwapInterval(0);

	TextUI* infos = new TextUI("alpha dev build : vA0.001.2", Vector3(0, 0, 0), Vector3(2, *ResY - 2, 0));
	infos->transform->SetScale(Vector3(0.4, 0.4, 0.4));
/*
	shader = new Shader(Map_VS_shader, Map_FS_shader);
	simpleDepthShader = new Shader(shadow_mapping_depthVS_shader, shadow_mapping_depthFS_shader);
	debugDepthQuad = new Shader(debug_quad_VS_shader, debug_quad_depth_FS_shader);
*/

	printf("Loading shader\n");
	shader = new Shader(Map_VS_shader, Map_FS_shader);
	printf("Loading skyboxShader\n");
	skyboxShader = new Shader(skybox_VS_shader, skybox_FS_shader);
	printf("Loading simpleDepthShader\n");
	simpleDepthShader = new Shader(a10_shadow_mapping_depth_VS_shader, a10_shadow_mapping_depth_FS_shader, a10_shadow_mapping_depth_GS_shader);
	printf("Loading simpleDepthShaderInstancied\n");
	simpleDepthShaderInstancied = new Shader(shadow_mapping_Instancing_VS_shader, shadow_mapping_Instancing_FS_shader, shadow_mapping_Instancing_GS_shader);
	printf("Loading debugDepthQuad\n");
	debugDepthQuad = new Shader(a10_debug_quad_VS_shader, a10_debug_quad_depth_FS_shader);
	printf("Loading debugCascadeShader\n");
	debugCascadeShader = new Shader(a10_debug_cascade_VS_shader, a10_debug_cascade_FS_shader);

	GenerateShadowBuffer();

    // configure UBO
    // --------------------

    glGenBuffers(1, &matricesUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4x4) * 16, nullptr, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, matricesUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);


	shader->use();
	shader->setFloat("heightScale", 0.1f);
	debugDepthQuad->use();
	debugDepthQuad->setInt("depthMap", 0);
}

double Time::GetTime() {
	return glfwGetTime() * 1000;
}

int Orientations[6] = 
{
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
	
};

unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(Orientations[i], 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			std::cout << "Loaded cubemap texture: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}


Shader* ImageShader;

void Image::InitImageRendering() {
	ImageShader = new Shader(imagesVS_shader, imagesFS_shader);
}

void Image::RenderImage(std::string path, int x, int y, int sizeX, int sizeY, int zIndex) {

	float relativeX = (((float)x / (float)*Renderer::ResX) * 2 - 1);
	float relativeY = -(((float)y / (float)*Renderer::ResY) * 2 - 1);
	float relativeSizeX = (float)sizeX / (float)(*Renderer::ResX * 2);
	float relativeSizeY = -(float)sizeY / (float)(*Renderer::ResY * 2);

	float vertices[] = {
		relativeX                , relativeY + relativeSizeY, (float)zIndex, 01, // left
		relativeX                , relativeY                , (float)zIndex, 00, // right
		relativeX + relativeSizeX, relativeY                , (float)zIndex, 10, // top

		relativeX                , relativeY + relativeSizeY, (float)zIndex, 01, // left
		relativeX + relativeSizeX, relativeY                , (float)zIndex, 10, // top
		relativeX + relativeSizeX, relativeY + relativeSizeY, (float)zIndex, 11  // right
	};

	static std::map<std::string, GLuint> Textures;

	ImageShader->use();

	if (Textures.find(path) == Textures.end()) {
		GLuint Texture = ImageLoader::LoadImageGL(path.c_str(), false);

		Textures.insert(std::pair<std::string, GLuint>(path, Texture));
	}

	GLuint Texture = Textures[path];

	glActiveTexture(GL_TEXTURE0);
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

	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
}

void Image::RenderImage(GLuint texture, int x, int y, int sizeX, int sizeY, int zIndex) {

	float relativeX = (((float)x / (float)*Renderer::ResX) * 2 - 1);
	float relativeY = -(((float)y / (float)*Renderer::ResY) * 2 - 1);
	float relativeSizeX = (float)sizeX / (float)(*Renderer::ResX * 2);
	float relativeSizeY = -(float)sizeY / (float)(*Renderer::ResY * 2);

	float vertices[] = {
		relativeX                , relativeY + relativeSizeY, (float)zIndex, 01, // left
		relativeX                , relativeY                , (float)zIndex, 00, // right
		relativeX + relativeSizeX, relativeY                , (float)zIndex, 10, // top

		relativeX                , relativeY + relativeSizeY, (float)zIndex, 01, // left
		relativeX + relativeSizeX, relativeY                , (float)zIndex, 10, // top
		relativeX + relativeSizeX, relativeY + relativeSizeY, (float)zIndex, 11  // right
	};
	/*
	printf("ResX : %d\nResY : %d\n", *Renderer::ResX, *Renderer::ResY);

	for (int i = 0; i < 24; i+=4)
	{
		printf("%f, %f, %f, %f\n", vertices[i], vertices[i + 1], vertices[i + 2], vertices[i + 3]);
	}*/

	ImageShader->use();
	glActiveTexture(GL_TEXTURE0);
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

	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

}


std::map<GLchar, Character> Characters;
unsigned int VAO, VBO;

void Text::InitTextRendering(std::string font_name) {
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
			y += 20;
			CurrentX = x;
			continue;
		}
		Character ch = Characters[c];

		float xpos = CurrentX + ch.Bearing.x * scale;
		float ypos = (*Renderer::ResY - y) - (ch.Size.y - ch.Bearing.y) * scale;

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

Vector3 ApplyRotation(Vector3 vec, float rotation) {
	Vector3 result;
	result.x = vec.x * cos(rotation) - vec.z * sin(rotation);
	result.y = vec.y;
	result.z = vec.x * sin(rotation) + vec.z * cos(rotation);
	return result;
}

glm::mat4 lightSpaceMatrix;

Shader* objShader = nullptr;

GLFWwindow* Renderer::GetWindow() {
	return window;
}

char * Name = nullptr;
Vector3 Value = Vector3(0, 0, 0);

std::vector<int> ShaderIDs;
void SetupShader(Shader * sh)
{

	int ShaderIDsCount = ShaderIDs.size();
	for (int i = 0; i < ShaderIDsCount; i++)
	{		
		if(ShaderIDs[i] == sh->ID)
			return;
	}
	ShaderIDs.push_back(sh->ID);
	if(Name != nullptr)
	{
		//printf("Set %s to %f %f %f\n", Name, Value.x, Value.y, Value.z);
		sh->setVec3(Name, Value);
	}

	sh->setMat4("projection", ProjectionMatrix);
	sh->setMat4("view", ViewMatrix);

	sh->setVec3("lightPos", Vector3(-lightDirGlobal.x * 100, -lightDirGlobal.y * 100, -lightDirGlobal.z * 100));
	sh->setVec3("lightDir", Vector3(-lightDirGlobal.x, -lightDirGlobal.y, -lightDirGlobal.z));
	sh->setVec3("viewPos", RaphEngine::camera->transform->GetPosition());
	sh->setFloat("farPlane", RaphEngine::camera->farPlane);
	sh->setMat4("lightSpaceMatrix", lightSpaceMatrix);

	sh->setInt("cascadeCount", shadowCascadeLevels.size());

	for (size_t i = 0; i < shadowCascadeLevels.size(); i++)
	{
		sh->setFloat(("cascadePlaneDistances[" + std::to_string(i) + "]").c_str(), shadowCascadeLevels[i]);
	}

	for(int i = 0; i < SAMPLE_SIZE; i++)
	{
		std::string name = "offsets[" + std::to_string(i) + "]";
		glUniform2f(glGetUniformLocation(sh->ID, name.c_str()), offsets[i].x, offsets[i].y);
	}

	//sh->setVec2Array("offsets", 64, offsets);

	const char* names[] = { "texture_diffuse", "texture_specular", "texture_normal", "texture_height", "shadowMap" };
	for (int i = 0; i < 5; i++)
	{
		sh->setInt(names[i], i);
	}

	glActiveTexture(GL_TEXTURE4);
	sh->setInt("shadowMap", 4);
	glBindTexture(GL_TEXTURE_2D_ARRAY, depthMap);
}

void CleanUpShaders()
{
	ShaderIDs.clear();
}

void RenderGameObject(Matrix4 ObjectModel, Mesh * mesh, Shader* shaderUse, int InstancesCount) {
	// int err = 0;

	if(mesh->vertices.size() == 0)
		return;

	//const char * names[] = { "texture_diffuse", "texture_specular", "texture_normal", "texture_height" };
	bool HaveTexture = false;
	for (size_t i = 0; i < mesh->textures.size(); i++)
	{
		if(mesh->textures[i].type == "texture_diffuse") HaveTexture = true;
		glActiveTexture(GL_TEXTURE0 + i);
		//shaderUse->setInt(mesh->textures[i].type.c_str(), i);
		glBindTexture(GL_TEXTURE_2D, mesh->textures[i].id);
	}
	shaderUse->setMat4("model", ObjectModel * mesh->ModelMatrix);
	shaderUse->setBool("HaveTexture", HaveTexture);
	shaderUse->setBool("HaveNormalMap", mesh->haveNormalMap);
	shaderUse->setBool("HaveSpecularMap", mesh->haveSpecularMap);
	shaderUse->setBool("HaveHeightMap", mesh->haveHeightMap);

	//glBindTexture(GL_TEXTURE_2D, depthMap);

	glBindVertexArray(mesh->vao);

	if(InstancesCount == 0)
		glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(mesh->indices.size()), GL_UNSIGNED_INT, 0);
	else
		glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(mesh->indices.size()), GL_UNSIGNED_INT, 0, InstancesCount);
	//glBindVertexArray(0);
	//glActiveTexture(GL_TEXTURE0);

}

void RenderGameObjectShadow(Mesh* mesh, int InstancesCount) {
	if (mesh->vertices.size() == 0)
		return;
	if (!mesh->castShadows)
		return;
	

	glBindVertexArray(mesh->vao);
	if(InstancesCount == 0)
		glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(mesh->indices.size()), GL_UNSIGNED_INT, 0);
	else
		glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(mesh->indices.size()), GL_UNSIGNED_INT, 0, InstancesCount);

	//glBindVertexArray(0);
	//glActiveTexture(GL_TEXTURE0);
}


void RenderObjects() {

	for (GameObject* go : GameObject::SpawnedGameObjects) {
		if (go->activeSelf)
		{

			Shader* RenderObjectShader = go->ObjectShader;
			if(RenderObjectShader == nullptr)
				continue;

			RenderObjectShader->use();

			int InstancesCount = 0;
			if(dynamic_cast<InstanciedGameObject*>(go) != nullptr)
			{
				InstanciedGameObject* InstanceGo = dynamic_cast<InstanciedGameObject*> (go);
				for (int i = 0; i < InstanceGo->instancesCount; i++)
				{
					std::string name = "ModelOffsets[" + std::to_string(i) + "]";
					//printf("setting this %dth shit to the correct fucking matrix\n");
					RenderObjectShader->setMat4(name.c_str(), InstanceGo->InstancesTransforms[i]->ModelMatrix);
				}
				InstancesCount = InstanceGo->instancesCount;
			}

			SetupShader(RenderObjectShader);
			//RenderObjectShader->use();
			
			// sh->setMat4("model", go->transform->ModelMatrix /* + mesh->ModelMatrix*/);
			std::vector<Mesh>* meshes = go->GetLODMesh(RaphEngine::camera->transform->GetPosition(), RaphEngine::camera->farPlane);
			if(meshes == nullptr)
				continue;
			int count = meshes->size();
			for (int i = 0; i < count; i++)
			{
				RenderGameObject(go->transform->ModelMatrix, &((*meshes)[i]), RenderObjectShader, InstancesCount);
			}
		}
	}
}

std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& projview)
{
    const auto inv = glm::inverse(projview);

    std::vector<glm::vec4> frustumCorners;
    for (unsigned int x = 0; x < 2; ++x)
    {
        for (unsigned int y = 0; y < 2; ++y)
        {
            for (unsigned int z = 0; z < 2; ++z)
            {
                const glm::vec4 pt = 
					inv * glm::vec4(
						2.0f * x - 1.0f,
						2.0f * y - 1.0f,
						2.0f * z - 1.0f,
						1.0f);
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }

    return frustumCorners;
}


std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view)
{
    return getFrustumCornersWorldSpace(proj * view);
}

glm::mat4 getLightSpaceMatrix(const float nearPlane, const float farPlane)
{
	const auto Proj = glm::perspective(
		glm::radians(RaphEngine::camera->fov),
		(float)(*Renderer::ResX) / (float)(*Renderer::ResY),
		nearPlane,
		farPlane
	);
    const auto corners = getFrustumCornersWorldSpace(Proj, ViewMatrix);

    glm::vec3 center = glm::vec3(0, 0, 0);
    for (const auto& v : corners)
    {
        center += glm::vec3(v);
    }
    center /= corners.size();



    const auto lightView = glm::lookAt(center, center + lightDirGlobal, glm::vec3(0.0f, 0.0f, 1.0f));

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();
    for (const auto& v : corners)
    {
        const auto trf = lightView * v;
        minX = std::min(minX, trf.x);
        maxX = std::max(maxX, trf.x);
        minY = std::min(minY, trf.y);
        maxY = std::max(maxY, trf.y);
        minZ = std::min(minZ, trf.z);
        maxZ = std::max(maxZ, trf.z);
    }

    // Tune this parameter according to the scene
    constexpr float zMult = 10.0f;
    if (minZ < 0)
    {
        minZ *= zMult;
    }
    else
    {
        minZ /= zMult;
    }
    if (maxZ < 0)
    {
        maxZ /= zMult;
    }
    else
    {
        maxZ *= zMult;
    }

    const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

	return lightProjection * lightView;
}

std::vector<glm::mat4> getLightSpaceMatrices()
{
    std::vector<glm::mat4> ret;
    for (size_t i = 0; i < shadowCascadeLevels.size() + 1; ++i)
    {
        if (i == 0)
        {
            ret.push_back(getLightSpaceMatrix(RaphEngine::camera->nearPlane, RaphEngine::camera->farPlane / shadowCascadeLevels[i]));
        }
        else if (i < shadowCascadeLevels.size())
        {
            ret.push_back(getLightSpaceMatrix(RaphEngine::camera->farPlane / shadowCascadeLevels[i - 1], RaphEngine::camera->farPlane / shadowCascadeLevels[i]));
        }
        else
        {
            ret.push_back(getLightSpaceMatrix(RaphEngine::camera->farPlane / shadowCascadeLevels[i - 1], RaphEngine::camera->farPlane));
        }
    }
    return ret;
}

void RenderObjectsShadows(Shader* sh, Shader* instanciedSh) {

	for (GameObject* go : GameObject::SpawnedGameObjects) {
		if (go->activeSelf)
		{
			int InstancesCount = 0;
			if(dynamic_cast<InstanciedGameObject*>(go) != nullptr)
			{
				sh = instanciedSh;
				sh->use();

				InstanciedGameObject* InstanceGo = dynamic_cast<InstanciedGameObject*> (go);
				for (int i = 0; i < InstanceGo->instancesCount; i++)
				{
					std::string name = "ModelOffsets[" + std::to_string(i) + "]";
					sh->setMat4(name.c_str(), InstanceGo->InstancesTransforms[i]->ModelMatrix);
				}
				InstancesCount = InstanceGo->instancesCount;
				
			}
			else
				sh->use();
			sh->setMat4("model", go->transform->ModelMatrix /* + mesh->ModelMatrix*/);
			std::vector<Mesh>* meshes = go->GetLODMesh(RaphEngine::camera->transform->GetPosition(), RaphEngine::camera->farPlane);
			if(meshes == nullptr)
				continue;
			int count = meshes->size();
			for (int i = 0; i < count; i++)
			{
				RenderGameObjectShadow(&((*meshes)[i]), InstancesCount);
			}
		}
	}
}


bool Renderer::IsKeyPressed(KeyCode key) {
	return glfwGetKey(window, (int)key) == GLFW_PRESS;
}

/*
void CalculateLights(glm::vec3 lightDir)
{
	float range = 50.0f;
	// 1. render depth of scene to texture (from light's perspective)
	// --------------------------------------------------------------
	glm::mat4 lightProjection, lightView;

	float near_plane = .1f, far_plane = range * 2.0f;

	//lightProjection = glm::perspective(glm::radians(165.f), (GLfloat)SHADOW_WIDTH / (GLfloat)SHADOW_HEIGHT, near_plane, far_plane); // note that if you use a perspective projection matrix you'll have to change the light position as the current light position isn't enough to reflect the whole scene
	lightProjection = glm::ortho(-range * 1.5f, range * 1.5f, -range * 1.5f, range * 1.5f, near_plane, far_plane);
	Vector3 pos = RaphEngine::camera->transform->GetPosition();
	if (RaphEngine::Player != nullptr)
	{
		pos = RaphEngine::Player->transform->GetPosition();
	}
	//std::cout << "Player pos: " << pos.x << " " << pos.y << " " << pos.z << std::endl;
	glm::vec3 lightPos(pos.x, pos.y, pos.z);// (-2.0f, 35.0f, -1.0f);

	lightPos -= range * lightDir;

	//std::cout << "Light pos: " << lightPos.x << " " << lightPos.y << " " << lightPos.z << std::endl;

	lightView = glm::lookAt(lightPos, lightPos + lightDir, glm::vec3(0.0, 1.0, 0.0));
	lightSpaceMatrix = lightProjection * lightView;

	// render scene from light's point of view
	simpleDepthShader->use();
	simpleDepthShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glActiveTexture(GL_TEXTURE0);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	
	RenderObjectsShadows(simpleDepthShader);
	glCullFace(GL_BACK); // don't forget to reset original culling face
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// reset viewport

	
}
*/


void CalculateLights(glm::vec3 lightDir)
{
	// 0. UBO setup
	
	const auto lightMatrices = getLightSpaceMatrices();
	if(showQuad)
		ProjectionMatrix = lightMatrices[0];
	glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
	for (size_t i = 0; i < lightMatrices.size(); i++)
	{
		glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(glm::mat4x4), sizeof(glm::mat4x4), &lightMatrices[i]);
		//std::cout << Mat4ToString(lightMatrices[i]);
	}
	glBindBuffer(GL_UNIFORM_BUFFER, 0);


	//simpleDepthShader->use();

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glClear(GL_DEPTH_BUFFER_BIT);
	//glCullFace(GL_FRONT);  // peter panning
	RenderObjectsShadows(simpleDepthShader, simpleDepthShaderInstancied);
	//glCullFace(GL_BACK);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

std::vector<GLuint> visualizerVAOs;
std::vector<GLuint> visualizerVBOs;
std::vector<GLuint> visualizerEBOs;
void drawCascadeVolumeVisualizers(const std::vector<glm::mat4>& lightMatrices, Shader* shader)
{
	//printf("Debug drawing...\n");
    visualizerVAOs.resize(8);
    visualizerEBOs.resize(8);
    visualizerVBOs.resize(8);

    const GLuint indices[] = {
        0, 2, 3,
        0, 3, 1,
        4, 6, 2,
        4, 2, 0,
        5, 7, 6,
        5, 6, 4,
        1, 3, 7,
        1, 7, 5,
        6, 7, 3,
        6, 3, 2,
        1, 5, 4,
        0, 1, 4
    };

    const glm::vec4 colors[] = {
        {1.0, 0.0, 0.0, 0.5f},
        {0.0, 1.0, 0.0, 0.5f},
        {0.0, 0.0, 1.0, 0.5f},
    };

    for (int i = 0; i < lightMatrices.size(); ++i)
    {
        const auto corners = getFrustumCornersWorldSpace(lightMatrices[i]);
        std::vector<glm::vec3> vec3s;
        for (const auto& v : corners)
        {
            vec3s.push_back(glm::vec3(v));
        }

        glGenVertexArrays(1, &visualizerVAOs[i]);
        glGenBuffers(1, &visualizerVBOs[i]);
        glGenBuffers(1, &visualizerEBOs[i]);

        glBindVertexArray(visualizerVAOs[i]);

        glBindBuffer(GL_ARRAY_BUFFER, visualizerVBOs[i]);
        glBufferData(GL_ARRAY_BUFFER, vec3s.size() * sizeof(glm::vec3), &vec3s[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, visualizerEBOs[i]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36 * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

        glBindVertexArray(visualizerVAOs[i]);
        shader->setVec4("color", colors[i % 3]);
        glDrawElements(GL_TRIANGLES, GLsizei(36), GL_UNSIGNED_INT, 0);

        glDeleteBuffers(1, &visualizerVBOs[i]);
        glDeleteBuffers(1, &visualizerEBOs[i]);
        glDeleteVertexArrays(1, &visualizerVAOs[i]);

        glBindVertexArray(0);
    }

    visualizerVAOs.clear();
    visualizerEBOs.clear();
    visualizerVBOs.clear();
}


float skyboxVertices[] = {
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

unsigned int skyboxVAO, skyboxVBO;
unsigned int cubemapTexture;
void InitSkybox()
{
    // skybox VAO
    
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	skyboxShader->use();
	skyboxShader->setInt("skybox", 0);

	cubemapTexture = loadCubemap(Renderer::skyboxTextures);
}

void RaphEngine::SetSkyBox(std::vector<std::string> skyboxTextures)
{
	Renderer::skyboxTextures = skyboxTextures;
	if (skyboxTextures.size() < 6)
	{
		printf("Skybox textures not enough, need 6 textures\n");
		return;
	}
	printf("Skybox set with %ld textures\n", skyboxTextures.size());
	if (skyboxTextures.size() > 6)
	{
		printf("Skybox textures too many, only using first 6 textures\n");
		skyboxTextures.resize(6);
	}
	InitSkybox();
}

void RenderSkyBox()
{
	if(Renderer::skyboxTextures.size() == 0)
		return;
	//printf("Rendering skybox\n");
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
	skyboxShader->use();
	glm::mat4 viewNoTranslate = glm::mat4(glm::mat3(ViewMatrix)); // remove translation from the view matrix
	skyboxShader->setMat4("view", viewNoTranslate);
	skyboxShader->setMat4("projection", ProjectionMatrix);
	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);

	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS); // set depth function back to default
}

void Renderer::StartFrameRender() {

	//float lightRotation = Time::GetTime() / 1000.0f;
	//glm::vec3 Ldir = glm::vec3(cos(lightRotation),  sin(lightRotation), -1);

	//glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	CalculateMat();
	
	CalculateLights(lightDirGlobal);
	// CalculateLights(glm::vec3(0, -1, -1));


	glViewport(0, 0, *ResX, *ResY);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	RenderSkyBox();
	
	RenderObjects();
	CleanUpShaders();
	
	if (lightMatricesCache.size() != 0)
	{
		//printf("Drawing mat\n");
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		debugCascadeShader->use();
		debugCascadeShader->setMat4("projection", ProjectionMatrix);
		debugCascadeShader->setMat4("view", ViewMatrix);
		drawCascadeVolumeVisualizers(lightMatricesCache, debugCascadeShader);
		glDisable(GL_BLEND);
	}


	Image::RenderImage(depthMap, 0, 0, 1000, 1000, 10);


	float range = 50.0f;
	//float near_plane = .1f, far_plane = range * 2.0f;

	
	debugDepthQuad->use();
	debugDepthQuad->setFloat("near_plane", RaphEngine::camera->nearPlane);
	debugDepthQuad->setFloat("far_plane", RaphEngine::camera->farPlane);
	debugDepthQuad->setInt("layer", debugLayer);
	glActiveTexture(GL_TEXTURE0);

	//GLuint Texture = ImageLoader::LoadImageGL("Assets/Textures/Logo.bmp", false);
	//glBindTexture(GL_TEXTURE_2D, Texture);
	glBindTexture(GL_TEXTURE_2D_ARRAY, depthMap);

	//if(showQuad)
	//	renderQuad();
}


void RaphEngine::PassShaderVector3(char* name, Vector3 value)
{
	//shader->setVec3(name, value);
	// printf("Setting %s to %f %f %f\n", name, value.x, value.y, value.z);
	Name = name;
	Value = value;
}

bool Renderer::RenderFrame() {

	// text rendering
	UIElement::DrawAllUI();
	

	glfwSwapBuffers(window);

	glfwPollEvents();

	glfwGetKey(window, GLFW_KEY_E);
	glfwGetKey(window, GLFW_KEY_F);
	glfwGetKey(window, GLFW_KEY_N);

	static int cPress = GLFW_RELEASE;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_RELEASE && cPress == GLFW_PRESS)
    {
		printf("C pressed!\n");
        lightMatricesCache = getLightSpaceMatrices();
    }
    cPress = glfwGetKey(window, GLFW_KEY_E);

    static int fPress = GLFW_RELEASE;
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE && fPress == GLFW_PRESS)
    {
        showQuad = !showQuad;
    }
    fPress = glfwGetKey(window, GLFW_KEY_F);

    static int plusPress = GLFW_RELEASE;
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_RELEASE && plusPress == GLFW_PRESS)
    {
        debugLayer++;
        if (debugLayer > shadowCascadeLevels.size())
        {
            debugLayer = 0;
        }
    }
    plusPress = glfwGetKey(window, GLFW_KEY_N);


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
