#pragma once

#include "Vertex.h"
#include <vector>
#include <string>

struct MeshData
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	
	std::string colorFilename;
	std::string diffuseFilename;
	std::string specularFilename;
	std::string normalFilename;

	std::string ddsColorFilename;
	std::string ddsDiffuseFilename;
	std::string ddsSpecularFilename;
};