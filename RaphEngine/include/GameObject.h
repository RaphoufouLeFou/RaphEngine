#pragma once
#include "LibManager.h"

#include "Vector.h"
#include <vector>
#include <string>
#ifdef RAPHENGINE_EXPORTS
#define GLM_ENABLE_EXPERIMENTAL
#include "Shader.h"
#ifdef _WIN32
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>
#else
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#endif
#endif
class Transform;
class Mesh;

class RAPHENGINE_API GameObject {
public:
	Transform* transform;
	GameObject* parent;
	GameObject** children;
	const char* name;
	const char* meshPath;
	bool smoothTextures;
	std::vector<Mesh> meshes;
	std::vector<Mesh> colliders;
	bool activeSelf;
	int layer;
	GameObject();
	~GameObject();
	void InitGO();
	virtual void Start() {}
	virtual void Update() {}

	static std::vector<GameObject*> SpawnedGameObjects;
};

class RAPHENGINE_API Transform {
private:
	Vector3 position;
	Vector3 rotation;
	Vector3 scale;
	void RecalculateMatrix();
public:
	GameObject* gameObject;
#ifdef RAPHENGINE_EXPORTS
	glm::mat4 ModelMatrix;
#endif
	void SetPosition(Vector3 position);
	void SetRotation(Vector3 rotation);
	void SetScale(Vector3 scale);
	Transform(GameObject* gameObject);
	Transform();
	Vector3 GetPosition();
	Vector3 GetRotation();
	Vector3 GetScale();
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
	bool haveNormalMap;
	bool haveSpecularMap;
	bool haveHeightMap;

	Matrix4 ModelMatrix;
	Vector3 InfSpehereCenter;
	float InfSphereRadius;

	unsigned int vao;
	bool castShadows;
	bool staticMesh;

	void ReclaculateNormals();
	void CalculateInflence();

	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures, bool autoNormal = false)
	{
		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;

		if(autoNormal)
			ReclaculateNormals();

		castShadows = true;
		staticMesh = false;

		GenerateBuffers();

	}
	unsigned int vbo, ebo;
private:
	void GenerateBuffers();
};

class RAPHENGINE_API Camera : public GameObject {
public:
	float fov;
	float nearPlane;
	float farPlane;
	Camera();
};
