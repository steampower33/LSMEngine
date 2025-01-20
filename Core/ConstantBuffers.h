#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include <DirectXMath.h>

using namespace DirectX;

#define MAX_LIGHTS 3

// 재질
struct Material {
	float albedo = 0.0f;
	float diffuse = 1.0f;
	float specular = 0.0f;
	float shininess = 1.0f;
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
	float d3[13];
};

__declspec(align(256)) struct MeshConstants {
	XMFLOAT4X4 world;

	XMFLOAT4X4 worldIT;

	Material material;
	UINT isUseTexture = 1;
	UINT isUseNormalMap = 1;
	UINT isUseHeightMap = 1;
	float heightScale = 0.0f;
	float d0[8];

	XMFLOAT4X4 d1;
};

__declspec(align(256)) struct TextureIndexConstants {
	UINT albedoIndex;
	UINT diffuseIndex;
	UINT specularIndex;
	UINT normalIndex;
	UINT heightIndex;
	float dummy[11];

	XMFLOAT4X4 dummy1;

	XMFLOAT4X4 dummy2;

	XMFLOAT4X4 dummy3;
};

__declspec(align(256)) struct CubemapIndexConstants {
	UINT cubemapEnvIndex;
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
	float strength = 0.0f;
	float exposure = 1.0f;
	float gamma = 2.2f;

	UINT index;
	UINT hightIndex;
	UINT lowIndex;
	UINT d[8];

	XMFLOAT4X4 dummy1;
	
	XMFLOAT4X4 dummy2;

	XMFLOAT4X4 dummy3;
};
