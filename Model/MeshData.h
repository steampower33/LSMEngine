#pragma once

#include "Vertex.h"
#include <vector>
#include <string>

struct MeshData
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	
	// 최대 8개의 파일 경로 존재
	std::string a;
};