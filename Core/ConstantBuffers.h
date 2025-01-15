#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include <DirectXMath.h>

using namespace DirectX;

#define MAX_LIGHTS 3

// 재질
struct Material {
	XMFLOAT3 ambient = { 0.1f, 0.1f, 0.1f };  // 12
	float shininess = 1.0f;           // 4
	XMFLOAT3 diffuse = { 0.5f, 0.5f, 0.5f };  // 12
	float dummy1 = 0.0f;              // 4
	XMFLOAT3 specular = { 0.5f, 0.5f, 0.5f }; // 12
	float dummy2 = 0.0f;              // 4
	XMFLOAT3 dummy3;						  // 12
	float dummy4 = 0.0f;			  // 4
};

// 조명
struct Light {
	// 순서와 크기 관계 주의 (16 바이트 패딩)
	XMFLOAT3 strength = { 1.0f, 1.0f, 1.0f };  // 12
	float fallOffStart = 0.0f;                     // 4
	XMFLOAT3 direction = { 0.0f, 0.0f, 1.0f }; // 12
	float fallOffEnd = 10.0f;                      // 4
	XMFLOAT3 position = { 0.0f, 0.0f, -2.0f }; // 12
	float spotPower = 1.0f;                        // 4
	XMFLOAT3 d1;							   // 12
	float d2 = 0.0f;;									   // 4
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
	UINT ambientIndex;
	UINT diffuseIndex;
	UINT specularIndex;
	float dummy[13];

	XMFLOAT4X4 dummy1;

	XMFLOAT4X4 dummy2;

	XMFLOAT4X4 dummy3;
};

__declspec(align(256)) struct CubemapIndexConstants {
	UINT cubemapAmbientIndex;
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
