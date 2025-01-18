#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include <DirectXMath.h>

using namespace DirectX;

#define MAX_LIGHTS 3

// 재질
struct Material {
	float ambient = 0.0f;
	float diffuse = 1.0f;
	float specular = 0.0f;
	float shininess = 1.0f;
	XMFLOAT4X3 d0;

	XMFLOAT4X4 d1;

	XMFLOAT4X4 d2;

	XMFLOAT4X4 d3;
};

// 조명
struct Light {
	XMFLOAT3 strength = { 1.0f, 1.0f, 1.0f };
	float fallOffStart = 0.0f;
	XMFLOAT3 direction = { 0.0f, 0.0f, 1.0f };
	float fallOffEnd = 10.0f;
	XMFLOAT3 position = { 0.0f, 0.0f, -2.0f };
	float spotPower = 1.0f;
	XMFLOAT3 d1;
	float d2 = 0.0f;
};

__declspec(align(512)) struct GlobalConstants
{
	XMFLOAT4X4 view;
	XMFLOAT4X4 proj;
	XMFLOAT4X4 d1;
	XMFLOAT4X4 d2;

	Light lights[MAX_LIGHTS];

	XMFLOAT3 eyeWorld;
	bool isUseTexture;
	bool dummy3[3];
	XMFLOAT4X3 dummy4;
};

__declspec(align(256)) struct MeshConstants {
	XMFLOAT4X4 world;

	XMFLOAT4X4 worldIT;

	Material material;

	XMFLOAT4X4 dummy1;
};

__declspec(align(256)) struct TextureIndexConstants {
	UINT colorIndex;
	UINT diffuseIndex;
	UINT specularIndex;
	float dummy[13];

	XMFLOAT4X4 dummy1;

	XMFLOAT4X4 dummy2;

	XMFLOAT4X4 dummy3;
};

__declspec(align(256)) struct CubemapIndexConstants {
	UINT cubemapColorIndex;
	UINT cubemapDiffuseIndex;
	UINT cubemapSpecularIndex;
	float dummy[13];

	XMFLOAT4X4 dummy1;

	XMFLOAT4X4 dummy2;

	XMFLOAT4X4 dummy3;
};

__declspec(align(256)) struct SamplingConstants {
	float dx;
	float dy;
	float threshold;
	float strength;

	UINT index;
	UINT hightIndex;
	UINT lowIndex;
	UINT d[9];

	XMFLOAT4X4 dummy1;
	
	XMFLOAT4X4 dummy2;

	XMFLOAT4X4 dummy3;
};
