#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include "d3dx12.h"
#include "ConstantBuffers.h"
#include "Helpers.h"
#include "Camera.h"
#include "PostProcess.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

class FrameResource
{
public:
	FrameResource(
		ComPtr<ID3D12Device>& device, float width, float height, UINT frameIndex);

	~FrameResource();

	void Update(
		shared_ptr<Camera>& camera,
		Light& m_lightFromGUI,
		XMFLOAT4& mirrorPlane,
		GlobalConstants& m_globalConstsBufferData,
		CubemapIndexConstants& cubemapIndexConsts);

	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_commandList;

	UINT64 m_fenceValue = 0;
	UINT m_frameIndex;

	// Constants
	ComPtr<ID3D12Resource> m_globalConstsUploadHeap;
	GlobalConstants m_globalConstsBufferData;
	UINT8* m_globalConstsBufferDataBegin = nullptr;

	ComPtr<ID3D12Resource> m_reflectGlobalConstsUploadHeap;
	GlobalConstants m_reflectGlobalConstsBufferData;
	UINT8* m_reflectGlobalConstsBufferDataBegin = nullptr;

	ComPtr<ID3D12Resource> m_cubemapIndexConstsUploadHeap;
	CubemapIndexConstants m_cubemapIndexConstsBufferData;
	UINT8* m_cubemapIndexConstsBufferDataBegin = nullptr;

	// SRV : shadowDepthOnly + depthOnly + resolved + fog + shadowMap
	ComPtr<ID3D12DescriptorHeap> m_srvHeap;

	ComPtr<ID3D12Resource> m_floatBuffers;
	ComPtr<ID3D12DescriptorHeap> m_floatRTVHeap;
	ComPtr<ID3D12Resource> m_floatDSBuffer;
	ComPtr<ID3D12DescriptorHeap> m_floatDSVHeap;

	ComPtr<ID3D12Resource> m_resolvedBuffers;
	ComPtr<ID3D12DescriptorHeap> m_resolvedRTVHeap;

	ComPtr<ID3D12Resource> m_depthOnlyDSBuffer;
	ComPtr<ID3D12DescriptorHeap> m_depthOnlyDSVHeap;
	ComPtr<ID3D12Resource> m_shadowMapDepthOnlyDSBuffer;
	ComPtr<ID3D12DescriptorHeap> m_shadowMapDepthOnlyDSVHeap;

	ComPtr<ID3D12Resource> m_fogBuffer;
	ComPtr<ID3D12DescriptorHeap> m_fogRTVHeap;

	float m_shadowMapWidth;
	float m_shadowMapHeight;
	ComPtr<ID3D12Resource> m_shadowMapBuffer;
	ComPtr<ID3D12DescriptorHeap> m_shadowMapRTVHeap;

	// PostProcess
	shared_ptr<PostProcess> m_postProcess;

private:
	void InitializeDescriptorHeaps(
		ComPtr<ID3D12Device>& device);

	float m_width;
	float m_height;

	UINT srvCnt = 0;

	UINT rtvSize;
	UINT cbvSrvSize;
};