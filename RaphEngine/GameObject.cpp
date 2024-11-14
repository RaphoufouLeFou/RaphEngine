#include "pch.h"

#include "include/GameObject.h"
#include "include/imageLoader.h"
#include "include/RaphEngine.h"

std::vector<GameObject*> GameObject::SpawnedGameObjects =
	std::vector<GameObject*>();

Shader* objShader = nullptr;

Shader * BuildShader(const char * vertexPath, const char * fragmentPath) {
	Shader * shader = new Shader(vertexPath, fragmentPath);
	if (shader == nullptr) {
		std::cout << "Failed to build shader" << std::endl;
	}
	return shader;
}

void GameObject::Init() {
	if (mesh == nullptr) {
		mesh = new Mesh();
	}
	if (objShader == nullptr) {
		objShader = BuildShader(vertexShader_shader, fragmentShader_shader);
	}
	
	mesh->shader = objShader;// BuildShader(VSShaderPath, FSShaderPath);
	mesh->castShadows = true;
	mesh->verticesCount = 0;
	mesh->generatedBuffers = false;
}

GameObject::GameObject() {
	SpawnedGameObjects.push_back(this);
	transform = new Transform();
	transform->position = { 0, 0, 0 };
	transform->rotation = { 0, 0, 0 };
	transform->scale = { 1, 1, 1 };
	transform->gameObject = this;
	parent = nullptr;
	children = nullptr;
	activeSelf = true;
	mesh = nullptr;
	name = "new GameObject";
}

void Mesh::LoadTexture(const char* texturePath, bool smooth) {
	texture = ImageLoader::LoadImageGL(texturePath, smooth);
}

GameObject::~GameObject() {
	delete transform;
	delete mesh;
}

void Transform::SetPosition(Vector3 position) {
	this->position = position;
}

void Transform::SetRotation(Vector3 rotation) {
	this->rotation = rotation;
}

void Transform::SetScale(Vector3 scale) {
	this->scale = scale;
}

Camera::Camera() : GameObject() {
	fov = 60.0f;
	nearPlane = 0.1f;
	farPlane = 1000.0f;
}