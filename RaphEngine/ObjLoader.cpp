#include "pch.h"

#include "include/ObjLoader.h"
#include <string>

bool loadOBJVerts(
	const char* path,
	Vector3** out_vertices,
	Vector2** out_uvs,
	Vector3** out_normals,
	int* verticesCount
) {

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<Vector3> temp_vertices;
	std::vector<Vector2> temp_uvs;
	std::vector<Vector3> temp_normals;


	FILE* file = fopen(path, "r");
	if (file == NULL) {
		printf("Impossible to open the file !\n");
		getchar();
		return false;
	}

	while (1) {

		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break; // EOF = End Of File. Quit the loop.

		// else : parse lineHeader

		if (strcmp(lineHeader, "v") == 0) {
			Vector3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			Vector2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			Vector3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9) {
				printf("File can't be read by our simple parser :-( Try exporting with other options\n");
				fclose(file);
				return false;
			}
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			uvIndices.push_back(uvIndex[0]);
			uvIndices.push_back(uvIndex[1]);
			uvIndices.push_back(uvIndex[2]);
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);
		}
		else {
			// Probably a comment, eat up the rest of the line
			char stupidBuffer[1000];
			fgets(stupidBuffer, 1000, file);
		}

	}
	int size = vertexIndices.size();
	*out_vertices = new Vector3[size];
	*out_uvs = new Vector2[size];
	*out_normals = new Vector3[size];
	// For each vertex of each triangle
	for (unsigned int i = 0; i < size; i++) {

		// Get the indices of its attributes
		unsigned int vertexIndex = vertexIndices[i];
		unsigned int uvIndex = uvIndices[i];
		unsigned int normalIndex = normalIndices[i];

		// Get the attributes thanks to the index
		Vector3 vertex = temp_vertices[vertexIndex - 1];
		Vector2 uv = temp_uvs[uvIndex - 1];
		Vector3 normal = temp_normals[normalIndex - 1];

		// Put the attributes in buffers
		(*out_vertices)[i] = vertex;
		(*out_uvs)[i] = uv;
		(*out_normals)[i] = normal;

	}
	*verticesCount = size;
	fclose(file);
	return true;
}

bool OBJLoader::loadOBJ(
	const char* path,
	GameObject* gameObject
) {
	Vector3 *vertices;
	Vector2 *uvs;
	Vector3 *normals;
	int verticesCount;

	if (!loadOBJVerts(path, &vertices, &uvs, &normals, &verticesCount)) {
		return false;
	}

	Mesh* mesh = new Mesh();
	mesh->vertices = vertices;
	mesh->uvs = uvs;
	mesh->normals = normals;
	mesh->shader = gameObject->mesh->shader;
	mesh->texture = gameObject->mesh->texture;
	mesh->verticesCount = verticesCount;
	mesh->castShadows = gameObject->mesh->castShadows;
	mesh->GenerateBuffers();

	gameObject->mesh = mesh;

	return true;
}

bool OBJLoader::loadOBJ(
	const char* path,
	Mesh* mesh
) {
	Vector3* vertices;
	Vector2* uvs;
	Vector3* normals;
	int verticesCount;

	if (!loadOBJVerts(path, &vertices, &uvs, &normals, &verticesCount)) {
		return false;
	}

	mesh->vertices = vertices;
	mesh->uvs = uvs;
	mesh->normals = normals;
	mesh->verticesCount = verticesCount;
	mesh->GenerateBuffers();

	return true;
}