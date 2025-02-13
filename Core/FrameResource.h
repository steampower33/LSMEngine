#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include "d3dx12.h"
#include "ConstantBuffers.h"
#include "Helpers.h"
#include "Camera.h"
#include "PostProcess.h"
#include "TextureManager.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

class FrameResource
{
public:
	FrameResource(
		ComPtr<ID3D12Device>& device, float width, float height, UINT frameIndex);

	~FrameResource();

	void InitializeDescriptorHeaps(
		ComPtr<ID3D12Device>& device,
		shared_ptr<TextureManager>& textureManager);

	void Update(
		shared_ptr<Camera>& camera,
		XMFLOAT4& mirrorPlane,
		GlobalConstants& m_globalConstsBufferData,
		GlobalConstants (&shadowGlobalConsts)[MAX_LIGHTS],
		CubemapIndexConstants& cubemapIndexConsts);

	static const UINT commandListCount = 1;

	ID3D12CommandList* m_batchSubmit[commandListCount];

	ComPtr<ID3D12CommandAllocator> m_commandAllocator[commandListCount];
	ComPtr<ID3D12GraphicsCommandList> m_commandList[commandListCount];

	UINT64 m_fenceValue = 0;
	UINT m_frameIndex;

	CD3DX12_VIEWPORT m_shadowViewport;
	CD3DX12_RECT m_shadowScissorRect;

	// Constants
	ComPtr<ID3D12Resource> m_globalConstsUploadHeap;
	GlobalConstants m_globalConstsData;
	UINT8* m_globalConstsDataBegin = nullptr;

	ComPtr<ID3D12Resource> m_shadowGlobalConstsUploadHeap[MAX_LIGHTS];
	GlobalConstants m_shadowGlobalConstsData[MAX_LIGHTS];
	UINT8* m_shadowGlobalConstsDataBegin[MAX_LIGHTS];

	ComPtr<ID3D12Resource> m_reflectConstsUploadHeap;
	GlobalConstants m_reflectConstsData;
	UINT8* m_reflectConstsDataBegin = nullptr;

	ComPtr<ID3D12Resource> m_cubemapIndexConstsUploadHeap;
	CubemapIndexConstants m_cubemapIndexConstsData;
	UINT8* m_cubemapIndexConstsDataBegin = nullptr;

	ComPtr<ID3D12Resource> m_floatBuffers;
	ComPtr<ID3D12DescriptorHeap> m_floatRTVHeap;
	ComPtr<ID3D12Resource> m_floatDSBuffer;
	ComPtr<ID3D12DescriptorHeap> m_floatDSVHeap;

	ComPtr<ID3D12Resource> m_resolvedBuffers;
	ComPtr<ID3D12DescriptorHeap> m_resolvedRTVHeap;

	ComPtr<ID3D12Resource> m_shadowDepthOnlyDSBuffer[MAX_LIGHTS];
	ComPtr<ID3D12DescriptorHeap> m_shadowDepthOnlyDSVHeap;
	ComPtr<ID3D12Resource> m_depthOnlyDSBuffer;
	ComPtr<ID3D12DescriptorHeap> m_depthOnlyDSVHeap;

	ComPtr<ID3D12Resource> m_fogBuffer;
	ComPtr<ID3D12DescriptorHeap> m_fogRTVHeap;

	ComPtr<ID3D12Resource> m_sceneBuffer;
	ComPtr<ID3D12DescriptorHeap> m_sceneRTVHeap;
	UINT m_sceneBufferIndex = -1;

	// PostProcess
	shared_ptr<PostProcess> m_postProcess;

private:

	float m_width;
	float m_height;

	float m_shadowWidth;
	float m_shadowHeight;

	UINT rtvSize;
	UINT cbvSrvSize;
	UINT dsvSize;
};