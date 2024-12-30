#pragma once

#include <vector>
#include "Vertex.h"

struct MeshData
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};