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
#endif

class Transform;
class Mesh;

class RAPHENGINE_API GameObject {
public:
	Transform* transform;
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

class RAPHENGINE_API Mesh {
public:
	std::vector<Vector3> vertices;
	std::vector<Vector3> normals;
	std::vector<Vector2> uvs;

	const char* texturePath;
	void LoadTexture(const char* texturePath, bool smooth);

#ifdef RAPHENGINE_EXPORTS
	Shader* shader;
	GLuint texture;
#endif
};

class RAPHENGINE_API Camera : public GameObject {
public:
	float fov;
	float nearPlane;
	float farPlane;
	Camera();
};
