#pragma once

#include <iostream>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"
#include "DirectXTex.h"

#include <string>
#include <wrl.h>
#include <shellapi.h>

#include "Helpers.h"

#include "DescriptorHeapAllocator.h"

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
		virtual void SizeChanged(UINT newWidth, UINT newHeight, bool minimized);

		void UpdateGUI();

		static HeapAllocator m_srvAlloc;

		UINT m_width;
		UINT m_height;

	private:
		struct Resolution
		{
			UINT Width;
			UINT Height;
		};

		float m_aspectRatio;
		static const UINT FrameCount = 3;
		static const UINT TextureWidth = 256;
		static const UINT TextureHeight = 256;
		static const UINT TexturePixelSize = 4;
		static const Resolution m_resolutionOptions[];
		static UINT m_resolutionIndex; // Index of the current scene rendering resolution from m_resolutionOptions.

		struct Vertex
		{
			Vertex(float x, float y, float z, float u, float v) : position(x, y, z), texcoord(u, v) {}
			XMFLOAT3 position;
			XMFLOAT2 texcoord;
		};

		struct SceneConstantBuffer
		{
			XMFLOAT4 offset;
			float padding[60];
		};

		static_assert((sizeof(SceneConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

		// Pipeline objects.
		CD3DX12_VIEWPORT m_viewport;
		CD3DX12_RECT m_scissorRect;
		ComPtr<ID3D12Device> m_device;
		ComPtr<ID3D12CommandQueue> m_commandQueue;
		ComPtr<IDXGISwapChain3> m_swapChain;
		ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		ComPtr<ID3D12DescriptorHeap> m_viewRTVHeap;
		ComPtr<ID3D12DescriptorHeap> m_viewSRVHeap;
		ComPtr<ID3D12DescriptorHeap> m_basicHeap;
		ComPtr<ID3D12DescriptorHeap> m_imguiHeap;
		UINT m_rtvDescriptorSize;
		ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
		ComPtr<ID3D12Resource> m_viewRenderTargets[FrameCount];
		ComPtr<ID3D12CommandAllocator> m_commandAllocator[FrameCount];
		ComPtr<ID3D12RootSignature> m_rootSignature;
		ComPtr<ID3D12PipelineState> m_pipelineState;
		ComPtr<ID3D12GraphicsCommandList> m_commandList;

		// App resources.
		ComPtr<ID3D12Resource> m_vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
		ComPtr<ID3D12Resource> m_indexBuffer;
		D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
		ComPtr<ID3D12Resource> m_constantBuffer;
		SceneConstantBuffer m_constantBufferData;
		UINT8* m_pCbvDataBegin;
		ComPtr<ID3D12Resource> m_texture;

		// Synchronization objects.
		UINT m_frameIndex;
		HANDLE m_fenceEvent;
		ComPtr<ID3D12Fence> m_fence[FrameCount];
		UINT64 m_fenceValue[FrameCount];

		bool m_windowVisible;
		bool m_windowedMode;

		void LoadPipeline();
		void LoadAssets();
		void LoadGUI();
		void PopulateCommandList();
		void WaitForPreviousFrame();
		void UpdateForSizeChange(UINT clientWidth, UINT clientHeight);
		void LoadSizeDependentResources();
		void RenderScene();

		// Get Adapter
		void GetHardwareAdapter(
			IDXGIFactory1* pFactory,
			IDXGIAdapter1** ppAdapter,
			bool requestHighPerformanceAdapter = false);

		// Adapter info.
		bool m_useWarpDevice;
	};

	

}