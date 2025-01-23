#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include <DirectXMath.h>

using namespace DirectX;

#define MAX_LIGHTS 3

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
	XMFLOAT3 eyeWorld = { 0.0f, 0.0f, 0.0f };
	float strengthIBL = 1.0f;
	int choiceEnvMap = 0;
	float envLodBias = 0.0f;
	float d03[10];
};

__declspec(align(256)) struct MeshConstants {
	XMFLOAT4X4 world;

	XMFLOAT4X4 worldIT;

	XMFLOAT3 albedoFactor = { 1.0f, 1.0f, 1.0f };
	float metallicFactor = 1.0f;
	float roughnessFactor = 1.0f;
	XMFLOAT3 emissionFactor = { 0.0f, 0.0f, 0.0f };
	float heightScale = 0.0f;
	UINT useAlbedoMap = 1;
	UINT useNormalMap = 0;
	UINT useHeightMap = 0;
	UINT useAOMap = 0;
	UINT useMetallicMap = 0;
	UINT useRoughnessMap = 0;
	UINT useEmissiveMap = 0;

	UINT invertNormalMapY = 0;
	float meshLodBias = 0.0f;
	float d0[2];

	XMFLOAT4X3 d1;
};

__declspec(align(256)) struct TextureIndexConstants {
	UINT albedoIndex = 0;
	UINT diffuseIndex = 0;
	UINT specularIndex = 0;
	UINT normalIndex = 0;
	UINT heightIndex = 0;
	UINT aoIndex = 0;
	UINT metallicIndex = 0;
	UINT roughnessIndex = 0;
	UINT emissiveIndex = 0;
	float dummy[7];

	XMFLOAT4X4 dummy1;

	XMFLOAT4X4 dummy2;

	XMFLOAT4X4 dummy3;
};

__declspec(align(256)) struct CubemapIndexConstants {
	UINT cubemapEnvIndex = 0;
	UINT cubemapDiffuseIndex = 0;
	UINT cubemapSpecularIndex = 0;
	UINT brdfIndex = 0;
	float dummy[12];

	XMFLOAT4X4 dummy1;

	XMFLOAT4X4 dummy2;

	XMFLOAT4X4 dummy3;
};

__declspec(align(256)) struct SamplingConstants {
	float dx = 0.0f;
	float dy = 0.0f;
	float strength = 0.0f;
	float exposure = 1.0f;
	float gamma = 2.2f;

	UINT index = 0;
	UINT hightIndex = 0;
	UINT lowIndex = 0;
	UINT d[8];

	XMFLOAT4X4 dummy1;
	
	XMFLOAT4X4 dummy2;

	XMFLOAT4X4 dummy3;
};
