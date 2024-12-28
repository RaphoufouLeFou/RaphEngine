#pragma once

#ifdef RAPHENGINE_EXPORTS
#define RAPHENGINE_API __declspec(dllexport)
#else
#define RAPHENGINE_API __declspec(dllimport)
#endif

#include "Vector.h"
#include <vector>
#ifdef RAPHENGINE_EXPORTS
#include "Shader.h"
#include <string>
#endif

class Transform;
class Mesh;

class RAPHENGINE_API GameObject {
public:
	Transform* transform;
	GameObject* parent;
	GameObject** children;
	const char* name;
	Mesh* mesh;
	bool activeSelf;
	GameObject();
	~GameObject();
	void Init();
	virtual void Start() {}
	virtual void Update() {}

	static std::vector<GameObject*> SpawnedGameObjects;
};

class RAPHENGINE_API Transform {
public:
	Vector3 position;
	Vector3 rotation;
	Vector3 scale;
	GameObject* gameObject;
	void SetPosition(Vector3 position);
	void SetRotation(Vector3 rotation);
	void SetScale(Vector3 scale);
};

struct RAPHENGINE_API Vertex {
	Vector3 Position;
	Vector3 Normal;
	Vector2 TexCoords;
	Vector3 Tangent;
	Vector3 Bitangent;
};

struct RAPHENGINE_API Texture {
	unsigned int id;
	std::string type;
	std::string path;
};

class RAPHENGINE_API Mesh {
public:
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;

	bool castShadows;
	bool staticMesh;

	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
	{
		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;

		GenerateBuffers();
	}
#ifdef RAPHENGINE_EXPORTS

	unsigned int vao, vbo, ebo;
private:
	void GenerateBuffers();
#endif
};

class RAPHENGINE_API Camera : public GameObject {
public:
	float fov;
	float nearPlane;
	float farPlane;
	Camera();
};
