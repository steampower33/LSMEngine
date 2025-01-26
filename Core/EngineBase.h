#pragma once

#include <iostream>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include "d3dx12.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include "DescriptorHeapAllocator.h"
#include "Camera.h"
#include "GraphicsCommon.h"
#include "ConstantBuffers.h"
#include "Helpers.h"
#include "TextureManager.h"
#include "FrameResource.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

class EngineBase
{
public:
	EngineBase();
	virtual ~EngineBase();

	virtual void Initialize() = 0;
	virtual void Update(float dt) = 0;
	virtual void Render() = 0;
	virtual void UpdateGUI() = 0;
	virtual void Destroy() = 0;


	shared_ptr<FrameResource> m_frameResources[FrameCount];
	FrameResource* m_pCurrFR;

	// Constants
	GlobalConstants m_globalConstsBufferData = {};
	GlobalConstants m_reflectGlobalConstsBufferData = {};
	CubemapIndexConstants m_cubemapIndexConstsBufferData = {};

	// Pipeline objects.
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<IDXGISwapChain3> m_swapChain;

	ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_srvHeap;
	ComPtr<ID3D12Resource> m_dsBuffer;
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

	ComPtr<ID3D12DescriptorHeap> m_imguiHeap;
	ComPtr<ID3D12DescriptorHeap> m_textureHeap;

	shared_ptr<Camera> m_camera;
	shared_ptr<TextureManager> m_textureManager;

	static HeapAllocator m_srvAlloc;

	UINT m_rtvDescriptorSize = 0;
	UINT m_cbvDescriptorSize = 0;

	float m_width;
	float m_height;

	float m_aspectRatio;
	UINT m_frameIndex = 0;

	float m_mousePosX = 0;
	float m_mousePosY = 0;
	float m_mouseDeltaX = 0;
	float m_mouseDeltaY = 0;
	float m_ndcX = 0;
	float m_ndcY = 0;

	bool m_isMouseMove = false;
	bool m_leftButton = false;
	bool m_rightButton = false;
	bool m_selected = false;
	bool m_dragStartFlag = false;


protected:
	void LoadPipeline();
	void InitializeDX12CoreComponents();
	void InitializeFence();
	void InitializeDescriptorHeaps();
	void LoadGUI();

	void WaitForGpu();
	void MoveToNextFrame();
	void WaitForPreviousFrame();

	// Synchronization objects.
	HANDLE m_fenceEvent = nullptr;
	ComPtr<ID3D12Fence> m_fence;

	bool multiFrame = true;

	// Get Adapter
	void GetHardwareAdapter(
		IDXGIFactory1* pFactory,
		IDXGIAdapter1** ppAdapter,
		bool requestHighPerformanceAdapter = false);

	// Adapter info.
	bool m_useWarpDevice = false;
};