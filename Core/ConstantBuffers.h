#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include <DirectXMath.h>

using namespace DirectX;

#define MAX_LIGHTS 2
#define LIGHT_OFF 0x00
#define LIGHT_DIRECTIONAL 0x01
#define LIGHT_POINT 0x02
#define LIGHT_SPOT 0x04
#define LIGHT_SHADOW 0x10

// 조명
struct Light
{
	XMFLOAT4X4 viewProj; // 그림자 렌더링에 필요
	XMFLOAT4X4 invProj; // 그림자 렌더링 디버깅용

	XMFLOAT3 radiance = { 5.0f, 5.0f, 5.0f };
	float fallOffStart = 0.0f;
	XMFLOAT3 direction = { 0.0f, 0.0f, 1.0f };
	float fallOffEnd = 20.0f;
	XMFLOAT3 position = { 0.0f, 0.0f, -2.0f };
	float spotPower = 6.0f;
	UINT type = LIGHT_OFF;
	float radius = 0.0f; // 반지름
	float d0;
	float d1;

	XMFLOAT4X4 d2;
};

__declspec(align(256)) struct GlobalConstants // 0
{
	XMFLOAT4X4 view;
	XMFLOAT4X4 proj;
	XMFLOAT4X4 viewProj;
	XMFLOAT4X4 invProj;
	
	XMFLOAT3 eyeWorld = { 0.0f, 0.0f, 0.0f };
	float strengthIBL = 1.0f;

	int choiceEnvMap = 0;
	float envLodBias = 0.0f;
	int fogMode = 1;
	float depthScale = 0.1f;

	float fogStrength = 0.0f;
	UINT depthOnlySRVIndex = 0;
	UINT shadowDepthOnlyStartIndex = 0;
	UINT resolvedSRVIndex = 0;

	UINT fogSRVIndex = 0;
	UINT isEnvEnabled = 1;
	float d01;
	float d02;
	
	XMFLOAT4X4 d03;
	XMFLOAT4X4 d04;
	XMFLOAT4X4 d05;

	Light light[MAX_LIGHTS];
};

__declspec(align(256)) struct MeshConstants // 1
{
	XMFLOAT4X4 world;

	XMFLOAT4X4 worldIT;

	XMFLOAT3 albedoFactor = { 1.0f, 1.0f, 1.0f };
	float metallicFactor = 1.0f;
	float roughnessFactor = 1.0f;
	XMFLOAT3 emissionFactor = { 0.0f, 0.0f, 0.0f };
	float heightScale = 0.0f;
	UINT useAlbedoMap = 0;
	UINT useNormalMap = 0;
	UINT useHeightMap = 0;
	UINT useAOMap = 0;
	UINT useMetallicMap = 0;
	UINT useRoughnessMap = 0;
	UINT useEmissiveMap = 0;

	UINT invertNormalMapY = 0;
	float meshLodBias = 0.0f;
	float d10;
	float d11;

	XMFLOAT4X3 d12;
};

__declspec(align(256)) struct TextureIndexConstants // 2
{
	UINT albedoIndex = 0;
	UINT diffuseIndex = 0;
	UINT specularIndex = 0;
	UINT normalIndex = 0;
	
	UINT heightIndex = 0;
	UINT aoIndex = 0;
	UINT metallicIndex = 0;
	UINT roughnessIndex = 0;
	
	UINT emissiveIndex = 0;
	float d20;
	float d21;
	float d22;

	XMFLOAT4 d23;

	XMFLOAT4X4 d24;

	XMFLOAT4X4 d25;

	XMFLOAT4X4 d26;
};

__declspec(align(256)) struct CubemapIndexConstants // 3
{
	UINT cubemapEnvIndex = 0;
	UINT cubemapDiffuseIndex = 0;
	UINT cubemapSpecularIndex = 0;
	UINT brdfIndex = 0;

	float d30[12];

	XMFLOAT4X4 d31;

	XMFLOAT4X4 d32;

	XMFLOAT4X4 d33;
};

__declspec(align(256)) struct SamplingConstants // 4
{
	float dx = 0.0f;
	float dy = 0.0f;
	float strength = 0.0f;
	float exposure = 1.0f;

	float gamma = 2.2f;
	UINT index = 0;
	UINT hightIndex = 0;
	UINT lowIndex = 0;

	XMFLOAT4 d40[2];

	XMFLOAT4X4 d41;
	
	XMFLOAT4X4 d42;

	XMFLOAT4X4 d43;
};
