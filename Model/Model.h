#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include "DirectXTex.h"

#include "MeshData.h"
#include "ConstantBuffers.h"
#include "Helpers.h"
#include "Mesh.h"
#include "GraphicsCommon.h"
#include "TextureManager.h"
#include <DirectXCollision.h>

using namespace std;
using Microsoft::WRL::ComPtr;
using namespace DirectX;

class Model
{
public:
	Model(
		ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList,
		ComPtr<ID3D12CommandQueue> commandQueue,
		const vector<MeshData>& meshDatas,
		CubemapIndexConstants& cubemapIndexConstsBufferData,
		shared_ptr<TextureManager> textureManager,
		XMFLOAT4 position);

	~Model();

	void Render(
		ComPtr<ID3D12Device>& device,
		ComPtr<ID3D12GraphicsCommandList>& commandList);

	void RenderSkybox(
		ComPtr<ID3D12Device>& device,
		ComPtr<ID3D12GraphicsCommandList>& commandList);

	void RenderNormal(
		ComPtr<ID3D12GraphicsCommandList>& commandList);

	void Update();
	void UpdateQuaternionAndTranslation(
		XMVECTOR& q, XMVECTOR& dragTranslation);
	void UpdateState();

	void OnlyCallConstsMemcpy();

	float m_radius;
	XMFLOAT4 m_position;
	XMFLOAT4X4 m_world;
	string m_key;

	bool m_useAlbedoMap = false;
	bool m_useNormalMap = false;
	bool m_useHeightMap = false;
	bool m_useAOMap = false;
	bool m_useMetallicMap = false;
	bool m_useRoughnessMap = false;
	bool m_useEmissiveMap = false;
	float m_scale = 1.0f;

	vector<shared_ptr<Mesh>> m_meshes;
	ComPtr<ID3D12Resource> m_meshConstsUploadHeap;
	MeshConstants m_meshConstsBufferData = {};
	UINT8* m_meshConstsBufferDataBegin;
	bool isVisible = true;

	shared_ptr<BoundingSphere> m_boundingSphere;

};