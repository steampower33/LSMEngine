#pragma once

#include <DirectXMath.h>

using namespace DirectX;

struct Vertex
{
	Vertex(float x, float y, float z, float u, float v, float nX, float nY, float nZ) :
		vertex(x, y, z), texcoord(u, v), normal(nX, nY, nZ) {
	}
	XMFLOAT3 vertex;
	XMFLOAT2 texcoord;
	XMFLOAT3 normal;
};