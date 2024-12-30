#pragma once

#include <DirectXMath.h>

using namespace DirectX;

struct Vertex
{
	Vertex() {}

	Vertex(float x, float y, float z, float u, float v, float nX, float nY, float nZ) :
		position(x, y, z), texcoord(u, v), normal(nX, nY, nZ) {
	}
	XMFLOAT3 position;
	XMFLOAT2 texcoord;
	XMFLOAT3 normal;
};