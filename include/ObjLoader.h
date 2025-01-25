#pragma once

#ifdef RAPHENGINE_EXPORTS
#define RAPHENGINE_API __declspec(dllexport)
#else
#define RAPHENGINE_API __declspec(dllimport)
#endif

#include <vector>
#include <string>
#include "Vector.h"
#include "GameObject.h"

#ifdef RAPHENGINE_EXPORTS

#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#endif

unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma);

class RAPHENGINE_API Model {
public:
    static std::vector<Texture> textures_loaded;
    std::vector<Mesh>    meshes;
    std::string directory;
    bool gammaCorrection;

    Model(std::string const& path, bool filter = true)
    {
        loadModel(path, filter);
    }
private:
	void loadModel(std::string const& path, bool filter);
#ifdef RAPHENGINE_EXPORTS
	void processNode(aiNode* node, const aiScene* scene, bool filter);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene, bool filter);
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, bool filter);
#endif
};
