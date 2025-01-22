#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include <DirectXMath.h>

using namespace DirectX;

#define MAX_LIGHTS 3

struct Material
{
	XMFLOAT3 albedo = { 1.0f, 1.0f, 1.0f };
	float metallic = 0.0f;
	float roughness = 0.0f;
};

// Α¶Έν
struct Light
{
	XMFLOAT3 radiance = { 1.0f, 1.0f, 1.0f };
	float fallOffStart = 0.0f;
	XMFLOAT3 direction = { 0.0f, 0.0f, 1.0f };
	float fallOffEnd = 20.0f;
	XMFLOAT3 position = { 0.0f, 0.0f, -2.0f };
	float spotPower = 100.0f;
	XMFLOAT3 d1;
	float d2;
};

__declspec(align(512)) struct GlobalConstants
{
	XMFLOAT4X4 view;
	XMFLOAT4X4 proj;
	XMFLOAT4X4 d1;
	XMFLOAT4X4 d2;

	Light light[MAX_LIGHTS];
	XMFLOAT3 eyeWorld;
	float d3[3];
};

__declspec(align(256)) struct MeshConstants {
	XMFLOAT4X4 world;

	XMFLOAT4X4 worldIT;

	Material material; // 20
	float heightScale = 0.0f;
	UINT useAlbedoMap = 1;
	UINT useNormalMap = 0;
	UINT useHeightMap = 0;
	UINT useAOMap = 0;
	UINT useMetallicMap = 0;
	UINT useRoughnessMap = 0;
	UINT useEmissiveMap = 0;
	UINT invertNormalMapY = 0;
	float d0[2];

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
	UINT emissiveIndex;
	float dummy[7];

	XMFLOAT4X4 dummy1;

	XMFLOAT4X4 dummy2;

	XMFLOAT4X4 dummy3;
};

__declspec(align(256)) struct CubemapIndexConstants {
	UINT cubemapEnvIndex;
	UINT cubemapDiffuseIndex;
	UINT cubemapSpecularIndex;
	UINT brdfIndex;
	float dummy[12];

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
