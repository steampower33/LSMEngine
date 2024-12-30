#pragma once

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <iostream>
#include <vector>

#include "MeshData.h"

class ModelLoader
{
public:
	void Load(std::string basePath, std::string filename);
	void ProcessNode(aiNode* node, const aiScene* scene, XMMATRIX parentTransform);
	MeshData ProcessMesh(aiMesh* mesh, const aiScene* scene);

public:
	std::string basePath;
	std::vector<MeshData> meshes;

};