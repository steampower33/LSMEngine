#pragma once

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "MeshData.h"
#include <filesystem>

using namespace std;

class ModelLoader
{
public:
	void Load(std::string basePath, std::string filename);
	void ProcessNode(aiNode* node, const aiScene* scene, XMMATRIX parentTransform);
	MeshData ProcessMesh(aiMesh* mesh, const aiScene* scene);
	std::string ReadFilename(aiMaterial* material, aiTextureType type);
	static string GetExtension(const string filename);

public:
	std::string basePath;
	std::vector<MeshData> meshes;
	bool m_isGLTF = false;
};