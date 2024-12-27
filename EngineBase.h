#pragma once

#include "Helpers.h"

#include "DescriptorHeapAllocator.h"
#include "Camera.h"

namespace EngineCore
{
	using namespace DirectX;
	using Microsoft::WRL::ComPtr;

	class EngineBase
	{
	public:
		EngineBase();
		virtual ~EngineBase();

		virtual void Initialize();
		virtual void Update(float dt) = 0;
		virtual void Render() = 0;
		virtual void UpdateGUI() = 0;

		static HeapAllocator m_srvAlloc;

		UINT m_width;
		UINT m_height;

		float m_aspectRatio;
		static const UINT FrameCount = 3;
		static const UINT TextureWidth = 256;
		static const UINT TextureHeight = 256;
		static const UINT TexturePixelSize = 4;


		Camera m_camera;

		ImVec2 m_scenePos = ImVec2(300, 5);
		ImVec2 m_sceneSize;
		ImVec2 m_lastMousePos;

		struct SceneConstantBuffer
		{
			XMFLOAT4 offset;
			XMFLOAT4X4 world;
			XMFLOAT4X4 view;
			XMFLOAT4X4 proj;
			float padding[12];
		};

		static_assert((sizeof(SceneConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

		// Pipeline objects.
		CD3DX12_VIEWPORT m_viewport;
		CD3DX12_RECT m_scissorRect;
		ComPtr<ID3D12Device> m_device;
		ComPtr<ID3D12CommandQueue> m_commandQueue;
		ComPtr<IDXGISwapChain3> m_swapChain;
		ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		ComPtr<ID3D12DescriptorHeap> m_sceneRTVHeap;
		ComPtr<ID3D12DescriptorHeap> m_sceneSRVHeap;
		ComPtr<ID3D12DescriptorHeap> m_basicHeap;
		ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
		ComPtr<ID3D12DescriptorHeap> m_imguiHeap;
		UINT m_rtvDescriptorSize;
		ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
		ComPtr<ID3D12Resource> m_sceneRenderTargets[FrameCount];
		ComPtr<ID3D12CommandAllocator> m_commandAllocator[FrameCount];
		ComPtr<ID3D12RootSignature> m_rootSignature;
		ComPtr<ID3D12PipelineState> m_pipelineState;
		ComPtr<ID3D12GraphicsCommandList> m_commandList;

		SceneConstantBuffer m_constantBufferData;

		ComPtr<ID3D12Resource> m_depthStencilBuffer;

	protected:
		void LoadPipeline();
		void LoadAssets();
		void LoadGUI();
		void WaitForPreviousFrame();
		
		// Synchronization objects.
		UINT m_frameIndex;
		HANDLE m_fenceEvent;
		ComPtr<ID3D12Fence> m_fence[FrameCount];
		UINT64 m_fenceValue[FrameCount];

		bool m_windowVisible;
		bool m_windowedMode;

		// Get Adapter
		void GetHardwareAdapter(
			IDXGIFactory1* pFactory,
			IDXGIAdapter1** ppAdapter,
			bool requestHighPerformanceAdapter = false);

		// Adapter info.
		bool m_useWarpDevice;
	};

	

}