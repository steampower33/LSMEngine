#pragma once

#include <DirectXMath.h>

using namespace DirectX;

struct Vertex
{
	Vertex() : position(0, 0, 0), texcoord(0, 0), normal(0, 0, 0), tangentModel(0, 0, 0) {}

	Vertex(float x, float y, float z, float u, float v, float nX, float nY, float nZ) :
		position(x, y, z), texcoord(u, v), normal(nX, nY, nZ), tangentModel(0, 0, 0) {
	}
	XMFLOAT3 position;
	XMFLOAT2 texcoord;
	XMFLOAT3 normal;
	XMFLOAT3 tangentModel;
};