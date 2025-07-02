#include "pch.h"

#include "include/ObjLoader.h"
#include "include/ImageLoader.h"
#include <string>

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

std::vector<Texture> Model::textures_loaded = std::vector<Texture>();

unsigned int TextureFromFile(const char* path, const std::string& directory, bool filter)
{
	std::string filename = std::string(path);
	if(directory != "")
		filename = directory + '/' + filename;
	int index = filename.find("Assets");
	if (index != -1)
		filename = filename.substr(index);
	unsigned int textureID;
	glGenTextures(1, &textureID);


	int width, height, nrComponents;
	unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);

	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		if (filter)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		std::cout << "Texture loaded at path: " << filename.c_str() << std::endl;
		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << filename.c_str() << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}


ImageData* Model::LoadImage(const char* path)
{
	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	ImageData* image = new ImageData();
	image->width = width;
	image->height = height;
	image->data = data;
	image->channels = nrComponents;
	return image;

}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, bool filter)
{
	std::vector<Texture> textures;
	for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		// check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
		bool skip = false;
		for (unsigned int j = 0; j < Model::textures_loaded.size(); j++)
		{
			if (std::strcmp(Model::textures_loaded[j].path.data(), str.C_Str()) == 0)
			{
				textures.push_back(Model::textures_loaded[j]);
				skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
				break;
			}
		}
		if (!skip)
		{   // if texture hasn't been loaded already, load it
			Texture texture;
			texture.id = TextureFromFile(str.C_Str(), this->directory, filter);
			texture.type = typeName;
			texture.path = str.C_Str();
			textures.push_back(texture);
			Model::textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
		}
	}
	return textures;
}

glm::vec4 CalculateInfluenceSphere(std::vector<Vertex> vertices, int verticesCount) {
	glm::vec3 center = glm::vec3(0.0f);
	for (int i = 0; i < verticesCount; i++) {
		center += glm::vec3(vertices[i].Position.x, vertices[i].Position.y, vertices[i].Position.z);
	}
	center /= verticesCount;
	float radius = 0.0f;
	for (int i = 0; i < verticesCount; i++) {
		float distance = glm::distance2(center, glm::vec3(vertices[i].Position.x, vertices[i].Position.y, vertices[i].Position.z));
		if (distance > radius) {
			radius = distance;
		}
	}
	return glm::vec4(center, radius);
}

void Mesh::CalculateInflence()
{
	Vector4 infSphere = CalculateInfluenceSphere(this->vertices, this->vertices.size());
	this->InfSpehereCenter = glm::vec3(infSphere.x, infSphere.y, infSphere.z);
	this->InfSphereRadius = infSphere.w;
}

std::string Mat4ToString(glm::mat4 mat)
{
    std::string str;
    str += "mat4(";
    for (int i = 0; i < 4; ++i)
    {
        str += "\n\t";
        for (int j = 0; j < 4; ++j)
        {
            str += std::to_string(mat[i][j]);
            if (j != 3)
            {
                str += ", ";
            }
        }
        str += "\n";
    }
    str += ")";
    return str;
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene, bool filter, glm::mat4 ModelMat)
{
	// data to fill
	std::vector<Vertex> vertices = std::vector<Vertex>();
	std::vector<unsigned int> indices = std::vector<unsigned int>();
	std::vector<Texture> textures = std::vector<Texture>();
	vertices.reserve(mesh->mNumVertices);

	// walk through each of the mesh's vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;
		Vector3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
		// positions
		vertex.Position.x = mesh->mVertices[i].x;
		vertex.Position.y = mesh->mVertices[i].y;
		vertex.Position.z = mesh->mVertices[i].z;
		int k = 0;
		// normals
		if (mesh->HasNormals())
		{
			vertex.Normal.x = mesh->mNormals[i].x;
			vertex.Normal.y = mesh->mNormals[i].y;
			vertex.Normal.z = mesh->mNormals[i].z;
		}
		// texture coordinates
		if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
		{
			Vector2 vec;
			// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
			// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
			vertex.TexCoords.x = mesh->mTextureCoords[0][i].x;
			vertex.TexCoords.y = mesh->mTextureCoords[0][i].y;
			// tangent
			vertex.Tangent.x = mesh->mTangents[i].x;
			vertex.Tangent.y = mesh->mTangents[i].y;
			vertex.Tangent.z = mesh->mTangents[i].z;
			// bitangent
			vertex.Bitangent.x = mesh->mBitangents[i].x;
			vertex.Bitangent.y = mesh->mBitangents[i].y;
			vertex.Bitangent.z = mesh->mBitangents[i].z;
		}
		else
			vertex.TexCoords = Vector2(0.0f, 0.0f);

		vertices.push_back(vertex);
	}
	// now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		// retrieve all indices of the face and store them in the indices vector
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}
	// process materials
	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	// we assume a convention for sampler names in the shaders. Each diffuse texture should be named
	// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
	// Same applies to other texture as the following list summarizes:
	// diffuse: texture_diffuseN
	// specular: texture_specularN
	// normal: texture_normalN

	// 1. diffuse maps
	std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", filter);
	textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
	// 2. specular maps
	std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", filter);
	textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
	// 3. normal maps
	std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal", filter);
	textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
	// 4. height maps
	std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_height", filter);
	textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

	Mesh mesh1(vertices, indices, textures);
	mesh1.ModelMatrix = ModelMat;
	printf("Generated model matix:\n%s\n", Mat4ToString(ModelMat).c_str());
	mesh1.haveNormalMap = normalMaps.size() > 0;
	mesh1.haveSpecularMap = specularMaps.size() > 0;
	mesh1.haveHeightMap = heightMaps.size() > 0;
	
	glm::vec4 infSphere = CalculateInfluenceSphere(vertices, mesh->mNumVertices);
	mesh1.InfSpehereCenter = glm::vec3(infSphere.x, infSphere.y, infSphere.z);
	mesh1.InfSphereRadius = infSphere.w;

	// return a mesh object created from the extracted mesh data
	return mesh1;
}

void Model::processNode(aiNode* node, const aiScene* scene, bool filter)
{
	// process each mesh located at the current node
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		// the node object only contains indices to index the actual objects in the scene. 
		// the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		glm::mat4 model = glm::mat4(1.0f);
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				model[i][j] = node->mTransformation[i][j];
		meshes.push_back(processMesh(mesh, scene, filter, model));
	}
	// after we've processed all of the meshes (if any) we then recursively process each of the children nodes
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene, filter);
	}

}

void Model::loadModel(std::string const& path, bool filter)
{
	// read file via ASSIMP
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	// check for errors
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
	{
		std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
		return;
	}
	// retrieve the directory path of the filepath
	std::string directory = path.substr(0, path.find_last_of('/'));

	// process ASSIMP's root node recursively
	processNode(scene->mRootNode, scene, filter);
}

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
