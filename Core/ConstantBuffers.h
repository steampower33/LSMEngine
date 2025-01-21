#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include <DirectXMath.h>

using namespace DirectX;

#define MAX_LIGHTS 3

// Α¶Έν
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

	float metallic = 0.0f;
	float roughness = 0.0f;
	float heightScale = 0.0f;
	UINT useAlbedoTexture = 0;
	UINT useNormalMap = 0;
	UINT useHeightMap = 0;
	UINT useAOMap = 0;
	UINT useMetallicMap = 0;
	UINT useRoughnessMap = 0;
	float d0[7];

	XMFLOAT4X4 d1;
};

__declspec(align(256)) struct TextureIndexConstants {
	UINT albedoIndex;
	UINT diffuseIndex;
	UINT specularIndex;
	UINT normalIndex;
	UINT heightIndex;
	UINT aoIndex;
	UINT metallicIndex;
	UINT roughnessIndex;
	float dummy[8];

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
