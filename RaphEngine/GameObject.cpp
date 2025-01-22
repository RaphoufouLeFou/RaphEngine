#include "pch.h"

#include "include/GameObject.h"
#include "include/imageLoader.h"
#include "include/RaphEngine.h"


std::vector<GameObject*> GameObject::SpawnedGameObjects =
	std::vector<GameObject*>();


GameObject::GameObject() {
	SpawnedGameObjects.push_back(this);
	std::cout << "GameObject created" << std::endl;
	transform = new Transform();
	transform->position = Vector3 (0, 0, 0);
	transform->rotation = Vector3(0, 0, 0);
	transform->scale = Vector3(1, 1, 1);
	transform->gameObject = this;
	parent = nullptr;
	children = nullptr;
	activeSelf = true;
	mesh = nullptr;
	name = "new GameObject";
}

GameObject::~GameObject() {
	delete transform;
	delete mesh;
}


void Mesh::GenerateBuffers() {

	// create buffers/arrays
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);
	// load data into vertex buffers
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// A great thing about structs is that their memory layout is sequential for all its items.
	// The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
	// again translates to 3/2 floats which translates to a byte array.
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	// set the vertex attribute pointers
	// vertex Positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	// vertex normals
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
	// vertex texture coords
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
	// vertex tangent
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
	// vertex bitangent
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
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