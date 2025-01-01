#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include "d3dx12.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include "DescriptorHeapAllocator.h"
#include "Camera.h"
#include "PSOManager.h"
#include "ConstantBuffers.h"
#include "Helpers.h"

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

	static HeapAllocator m_srvAlloc;

	UINT m_width;
	UINT m_height;

	float m_aspectRatio;
	static const UINT FrameCount = 3;

	Camera m_camera;
	PSOManager m_psoManager;

	float m_mousePosX;
	float m_mousePosY;
	float m_mouseDeltaX;
	float m_mouseDeltaY;

	bool m_isMouseMove;

	// Pipeline objects.
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	ComPtr<ID3D12DescriptorHeap> m_cbvHeap; 
	ComPtr<ID3D12DescriptorHeap> m_textureHeap;
	ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	ComPtr<ID3D12CommandAllocator> m_commandAllocator[FrameCount];
	ComPtr<ID3D12GraphicsCommandList> m_commandList;

	UINT m_rtvDescriptorSize;
	UINT m_cbvDescriptorSize;

	ComPtr<ID3D12Resource> m_depthStencilBuffer;

	ComPtr<ID3D12Resource> m_globalConstsUploadHeap;
	GlobalConstants m_globalConstsBufferData;
	UINT8* m_globalConstsBufferDataBegin;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_textureHandle;

protected:
	void LoadPipeline();
	void LoadGUI();
	void WaitForPreviousFrame();

	// Synchronization objects.
	UINT m_frameIndex;
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence[FrameCount];
	UINT64 m_fenceValue[FrameCount];

	// Get Adapter
	void GetHardwareAdapter(
		IDXGIFactory1* pFactory,
		IDXGIAdapter1** ppAdapter,
		bool requestHighPerformanceAdapter = false);

	// Adapter info.
	bool m_useWarpDevice;
};