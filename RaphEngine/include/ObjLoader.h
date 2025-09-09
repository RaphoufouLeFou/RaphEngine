#pragma once
#include "LibManager.h"

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

typedef struct ImageData
{
    unsigned char* data;
    int width;
    int height;
    int channels;
} ImageData;

RAPHENGINE_API unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma);


class RAPHENGINE_API Model {
public:
    static ImageData* MeshLoadImage(const char* path);
    static std::vector<Texture> textures_loaded;
    std::vector<Mesh>    meshes;
    std::string directory;
    bool gammaCorrection;

    Model(std::string const& path, bool filter = true)
    {
        meshes = std::vector<Mesh>();
        loadModel(path, filter);
    }
private:
	void loadModel(std::string const& path, bool filter);
#ifdef RAPHENGINE_EXPORTS
	void processNode(aiNode* node, const aiScene* scene, bool filter);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene, bool filter, glm::mat4 ModelMat);
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, bool filter);
#endif
};
