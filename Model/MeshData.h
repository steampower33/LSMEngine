#pragma once

#include "Vertex.h"
#include <vector>
#include <string>

struct MeshData
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	
	// �ִ� 8���� ���� ��� ����
	std::string a;
};