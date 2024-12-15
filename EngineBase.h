#pragma once

#include <iostream>

#include <shlobj.h>
#include <strsafe.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"

#include <string>
#include <wrl.h>
#include <shellapi.h>

#include "Helpers.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace EngineCore
{
	class EngineBase
	{
	public:
		EngineBase(UINT width, UINT height, std::wstring name);
		virtual ~EngineBase();

		virtual void Init();
		virtual void Update();
		virtual void Render();
		virtual void Destroy();

		static std::wstring GetLatestWinPixGpuCapturerPath();

	private:
		std::wstring m_assetsPath;
		float m_aspectRatio;
		static const UINT FrameCount = 3;

		struct Vertex
		{
			XMFLOAT3 position;
			XMFLOAT4 color;
		};

		// width, height
		UINT m_width;
		UINT m_height;

		// Pipeline objects.
		CD3DX12_VIEWPORT m_viewport;
		CD3DX12_RECT m_scissorRect;
		ComPtr<ID3D12Device> m_device;
		ComPtr<ID3D12CommandQueue> m_commandQueue;
		ComPtr<IDXGISwapChain3> m_swapChain;
		ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		UINT m_rtvDescriptorSize;
		ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
		ComPtr<ID3D12CommandAllocator> m_commandAllocator;
		ComPtr<ID3D12RootSignature> m_rootSignature;
		ComPtr<ID3D12PipelineState> m_pipelineState;
		ComPtr<ID3D12GraphicsCommandList> m_commandList;

		// App resources.
		ComPtr<ID3D12Resource> m_vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

		// Synchronization objects.
		UINT m_frameIndex;
		HANDLE m_fenceEvent;
		ComPtr<ID3D12Fence> m_fence;
		UINT64 m_fenceValue;

		std::wstring GetAssetFullPath(LPCWSTR assetName);

		void LoadPipeline();
		void LoadAssets();
		void PopulateCommandList();
		void WaitForPreviousFrame();

		// Get Adapter
		void GetHardwareAdapter(
			IDXGIFactory1* pFactory,
			IDXGIAdapter1** ppAdapter,
			bool requestHighPerformanceAdapter = false);

		// Adapter info.
		bool m_useWarpDevice;
	};
}