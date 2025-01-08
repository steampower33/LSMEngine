#pragma once

#include "Vertex.h"
#include <vector>
#include <string>

struct MeshData
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	
	std::string ambientFilename;
	std::string diffuseFilename;
	std::string specularFilename;

	std::string ddsAmbientFilename;
	std::string ddsDiffuseFilename;
	std::string ddsSpecularFilename;
};