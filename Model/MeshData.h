#pragma once

#include "Vertex.h"
#include <vector>
#include <string>

struct MeshData
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	
	std::string albedoFilename;
	std::string diffuseFilename;
	std::string specularFilename;
	std::string normalFilename;
	std::string heightFilename;
	std::string aoFilename;
	std::string metallicFilename;
	std::string roughnessFilename;

	std::string cubeEnvFilename;
	std::string cubeDiffuseFilename;
	std::string cubeSpecularFilename;
	std::string cubeBrdfFilename;
};