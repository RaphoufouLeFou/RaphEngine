#pragma once

#ifdef RAPHENGINE_EXPORTS
#define RAPHENGINE_API __declspec(dllexport)
#else
#define RAPHENGINE_API __declspec(dllimport)
#endif

#include <vector>
#include "Vector.h"
#include "GameObject.h"

class RAPHENGINE_API OBJLoader
{
public:
	static bool loadOBJ(
		const char* path,
		std::vector<Vector3>& out_vertices,
		std::vector<Vector2>& out_uvs,
		std::vector<Vector3>& out_normals
	);

	static bool loadOBJ(
		const char* path,
		GameObject* gameObject
	);

	static bool loadOBJ(
		const char* path,
		Mesh* mesh
	);
};
