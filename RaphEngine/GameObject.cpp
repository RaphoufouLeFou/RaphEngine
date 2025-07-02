#include "pch.h"

#include "include/GameObject.h"
//#include "include/imageLoader.h"
#include "include/RaphEngine.h"


std::vector<GameObject*> GameObject::SpawnedGameObjects = std::vector<GameObject*>();


GameObject::GameObject() {
	SpawnedGameObjects.push_back(this);
	transform = new Transform(this);
	ObjectShader = nullptr;
	parent = nullptr;
	children = nullptr;
	layer = 0;
	activeSelf = true;
	smoothTextures = true;
	for (int i = 0; i < 16; i++)
	{
		meshPaths[i] = nullptr;
	}
	
	name = "new GameObject";
}

GameObject::~GameObject() {
	delete transform;
}

InstanciedGameObject::InstanciedGameObject() 
{
	instancesCount = 0;
}

void GameObject::InitGO() {
	if(LODsCount > 16) LODsCount = 16;
	for (int i = 0; i < LODsCount; i++)
	{
		const char * meshPath = meshPaths[i];
		if(meshPath == nullptr)
			continue;
		printf("Loading model %d of %s\n", i, name);
		Model* model = new Model(meshPath, smoothTextures);
		LODs[i].insert(LODs[i].end(), model->meshes.begin(), model->meshes.end());
	}
}

std::vector<Mesh>* GameObject::GetLODMesh(Vector3 cameraPos, float maxDist)
{
	if(LODsCount <= 0) return nullptr;
	float distToCamera = glm::distance(cameraPos, transform->GetPosition());
	float stepDistance = maxDist / (float)LODsCount;
	int index = (int)(distToCamera / stepDistance);
	if(index > LODsCount) index = LODsCount;
	return &LODs[index];
}

void Mesh::ReclaculateNormals() {

	int triCount = indices.size() / 3;
	int vertexCount = vertices.size();

	printf("Recalculating normals for %d vertices and %d triangles\n", vertexCount, triCount);

	for (int i = 0; i < vertexCount; i++)
	{
		vertices[i].Normal = Vector3(0, 0, 0);
		vertices[i].Tangent = Vector3(0, 0, 0);
		vertices[i].Bitangent = Vector3(0, 0, 0);
	}

	for(int i = 0; i < triCount; i++)
	{
		Vertex *v1 = &vertices[indices[i * 3 + 0]];
		Vertex *v2 = &vertices[indices[i * 3 + 1]];
		Vertex *v3 = &vertices[indices[i * 3 + 2]];

		Vector3 edge1 = v2->Position - v3->Position;
		Vector3 edge2 = v2->Position - v1->Position;

		Vector3 normal = glm::normalize(glm::cross(edge1, edge2));

		glm::vec2 deltaUV1 = v3->TexCoords - v1->TexCoords;
		glm::vec2 deltaUV2 = v2->TexCoords - v1->TexCoords;

		float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

		glm::vec3 tangent, bitangent;

		tangent = f * (deltaUV2.y * edge1 - deltaUV1.y * edge2);
		bitangent = f * (-deltaUV2.x * edge1 + deltaUV1.x * edge2);

		v1->Normal += normal;
		v2->Normal += normal;
		v3->Normal += normal;

		v1->Tangent += tangent;
		v2->Tangent += tangent;
		v3->Tangent += tangent;

		v1->Bitangent += bitangent;
		v2->Bitangent += bitangent;
		v3->Bitangent += bitangent;
	}

	for (int i = 0; i < vertexCount; i++)
	{
		vertices[i].Normal = glm::normalize(vertices[i].Normal);
		vertices[i].Tangent = glm::normalize(vertices[i].Tangent);
		vertices[i].Bitangent = glm::normalize(vertices[i].Bitangent);
	}
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

void Transform::RecalculateMatrix()
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, position);
	model = model * glm::toMat4(glm::quat(glm::radians(rotation)));
	model = glm::scale(model, scale);
	ModelMatrix = model;
}

void Transform::SetPosition(Vector3 position) {
	this->position = position;
	RecalculateMatrix();
}

void Transform::SetRotation(Vector3 rotation) {
	this->rotation = rotation;
	RecalculateMatrix();
}

void Transform::SetScale(Vector3 scale) {
	this->scale = scale;
	RecalculateMatrix();
}

Transform::Transform(GameObject* gameObject)
{
	this->gameObject = gameObject;
	this->position = Vector3(0, 0, 0);
	this->rotation = Vector3(0, 0, 0);
	this->scale = Vector3(1, 1, 1);
	RecalculateMatrix();
}

Transform::Transform(Vector3 Position)
{
	this->gameObject = nullptr;
	this->position = Position;
	this->rotation = Vector3(0, 0, 0);
	this->scale = Vector3(1, 1, 1);
	RecalculateMatrix();
}

Transform::Transform()
{
	this->gameObject = nullptr;
	this->position = Vector3(0, 0, 0);
	this->rotation = Vector3(0, 0, 0);
	this->scale.x = 1;
	this->scale.y = 1;
	this->scale.z = 1;
}

Vector3 Transform::GetPosition()
{
	return position;
}
Vector3 Transform::GetRotation()
{
	return rotation;
}
Vector3 Transform::GetScale()
{
	return scale;
}

Camera::Camera() : GameObject() {
	fov = 60.0f;
	nearPlane = 0.1f;
	farPlane = 20000.0f;
}