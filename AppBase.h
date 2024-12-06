#pragma once

#include "stdafx.h"

#include <shlobj.h>
#include <strsafe.h>

namespace EngineCore
{

	class AppBase
	{

	public:
		AppBase();
		virtual ~AppBase();
		
		bool Run();

		// 콜백 함수
		LRESULT MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		std::wstring GetLatestWinPixGpuCapturerPath();

		virtual bool Initialize();
		virtual void UpdateGUI() = 0;
		virtual void Update() = 0;
		virtual void Render() = 0;

		bool InitializeWindow();
		bool InitD3D();
		void Cleanup();
		void WaitForPreviousFrame();

		bool CreateDevice();
		bool CreateCommandQueue();
		bool CreateSwapChain();
		bool CreateRenderTargetViews();
		bool CreateCommandTools();
		bool CreateRootSignature();
		bool CreateVertexShader();
		bool CreatePixelShader();
		bool CreateDepthStencilBuffer();
		bool CreatePipelineStateObject();
		bool CreateVertexBuffer();
		bool CreateIndexBuffer();
		bool CreateConstantBuffer();
		bool CreateTexture();
		bool SetCommandQueueFence();
		void SetViewport();
		void SetScissorRect();

		void UpdatePipeline();

	private:
		struct Vertex {
			Vertex(float x, float y, float z, float u, float v) : pos(x, y, z), texCoord(u, v) {}
			XMFLOAT3 pos;
			XMFLOAT2 texCoord;
		};

		HWND m_hwnd = NULL;
		LPCTSTR m_windowName;
		LPCTSTR m_windowTitle;

		int m_width;
		int m_height;
		bool m_fullScreen;
		bool m_running;

		static const int m_frameBufferCount = 3; // number of buffers we want, 2 for double buffering, 3 for tripple buffering

		ComPtr<IDXGIFactory4> m_dxgiFactory;
		ComPtr<IDXGIAdapter1> m_adapter; // adapters are the graphics card (this includes the embedded graphics on the motherboard)
		ComPtr<ID3D12Device> m_device; // direct3d device

		DXGI_MODE_DESC m_backBufferDesc = {}; // this is to describe our display mode
		DXGI_SAMPLE_DESC m_sampleDesc = {}; // describe our multi-sampling. We are not multi-sampling, so we set the count to 1 (we need at least one sample of course)
		DXGI_SWAP_CHAIN_DESC m_swapChainDesc = {};
		ComPtr<IDXGISwapChain> m_tempSwapChain;
		ComPtr<IDXGISwapChain3> m_swapChain; // swapchain used to switch between render targets

		ComPtr<ID3D12CommandQueue> m_commandQueue; // container for command lists
		ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap; // a descriptor heap to hold resources like the render targets
		ComPtr<ID3D12Resource> m_renderTargets[m_frameBufferCount]; // number of render targets equal to buffer count
		ComPtr<ID3D12CommandAllocator> m_commandAllocator[m_frameBufferCount]; // we want enough allocators for each buffer * number of threads (we only have one thread)
		ComPtr<ID3D12GraphicsCommandList> m_commandList; // a command list we can record commands into, then execute them to render the frame
		ComPtr<ID3D12Fence> m_fence[m_frameBufferCount]; // an object that is locked while our command list is being executed by the gpu. We need as many 
		// as we have allocators (more if we want to know when the gpu is finished with an asset)

		HANDLE m_fenceEvent; // a handle to an event when our fence is unlocked by the gpu
		UINT64 m_fenceValue[m_frameBufferCount]; // this value is incremented each frame. each fence will have its own value

		int m_frameIndex; // current rtv we are on
		int m_rtvDescriptorSize; // size of the rtv descriptor on the device (all front and back buffers will be the same size)

		ComPtr<ID3D12RootSignature> m_rootSignature; // root signature defines data shaders will access

		ComPtr<ID3DBlob> m_errorBuff; // a buffer holding the error data if any

		ComPtr<ID3DBlob> m_vertexShader; // d3d blob for holding vertex shader bytecode
		D3D12_SHADER_BYTECODE m_vertexShaderBytecode = {};

		ComPtr<ID3DBlob> m_pixelShader;
		D3D12_SHADER_BYTECODE m_pixelShaderBytecode = {};

		ComPtr<ID3D12PipelineState> m_pipelineStateObject; // pso containing a pipeline state

		ComPtr<ID3D12Resource> m_vertexBuffer; // a default buffer in GPU memory that we will load vertex data for our triangle into
		D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView; // a structure containing a pointer to the vertex data in gpu memory
		// the total size of the buffer, and the size of each element (vertex)

		ComPtr<ID3D12Resource> m_indexBuffer; // a default buffer in GPU memory that we will load index data for our triangle into
		D3D12_INDEX_BUFFER_VIEW m_indexBufferView; // a structure holding information about the index buffer

		ComPtr<ID3D12Resource> m_depthStencilBuffer; // This is the memory for our depth buffer. it will also be used for a stencil buffer in a later tutorial
		ComPtr<ID3D12DescriptorHeap> m_dsDescriptorHeap; // This is a heap for our depth/stencil buffer descriptor

		D3D12_VIEWPORT viewport; // area that output from rasterizer will be stretched to.

		D3D12_RECT scissorRect; // the area to draw in. pixels outside that area will not be drawn onto

		// this is the structure of our constant buffer.
		struct ConstantBufferPerObject {
			XMFLOAT4X4 wvpMat;
		};

		// Constant buffers must be 256-byte aligned which has to do with constant reads on the GPU.
		// We are only able to read at 256 byte intervals from the start of a resource heap, so we will
		// make sure that we add padding between the two constant buffers in the heap (one for cube1 and one for cube2)
		// Another way to do this would be to add a float array in the constant buffer structure for padding. In this case
		// we would need to add a float padding[50]; after the wvpMat variable. This would align our structure to 256 bytes (4 bytes per float)
		// The reason i didn't go with this way, was because there would actually be wasted cpu cycles when memcpy our constant
		// buffer data to the gpu virtual address. currently we memcpy the size of our structure, which is 16 bytes here, but if we
		// were to add the padding array, we would memcpy 64 bytes if we memcpy the size of our structure, which is 50 wasted bytes
		// being copied.
		int ConstantBufferPerObjectAlignedSize = (sizeof(ConstantBufferPerObject) + 255) & ~255;

		ConstantBufferPerObject cbPerObject; // this is the constant buffer data we will send to the gpu 
		// (which will be placed in the resource we created above)

		ID3D12Resource* constantBufferUploadHeaps[m_frameBufferCount]; // this is the memory on the gpu where constant buffers for each frame will be placed

		UINT8* cbvGPUAddress[m_frameBufferCount]; // this is a pointer to each of the constant buffer resource heaps

		XMFLOAT4X4 cameraProjMat; // this will store our projection matrix
		XMFLOAT4X4 cameraViewMat; // this will store our view matrix

		XMFLOAT4 cameraPosition; // this is our cameras position vector
		XMFLOAT4 cameraTarget; // a vector describing the point in space our camera is looking at
		XMFLOAT4 cameraUp; // the worlds up vector

		XMFLOAT4X4 cube1WorldMat; // our first cubes world matrix (transformation matrix)
		XMFLOAT4X4 cube1RotMat; // this will keep track of our rotation for the first cube
		XMFLOAT4 cube1Position; // our first cubes position in space

		XMFLOAT4X4 cube2WorldMat; // our first cubes world matrix (transformation matrix)
		XMFLOAT4X4 cube2RotMat; // this will keep track of our rotation for the second cube
		XMFLOAT4 cube2PositionOffset; // our second cube will rotate around the first cube, so this is the position offset from the first cube

		int numCubeIndices; // the number of indices to draw the cube

		ID3D12Resource* textureBuffer; // the resource heap containing our texture
		ID3D12Resource* textureBuffer1; // the resource heap containing our texture

		int LoadImageDataFromFile(BYTE** imageData, D3D12_RESOURCE_DESC& resourceDescription, LPCWSTR filename, int& bytesPerRow);

		DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID);
		WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID);
		int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat);

		ID3D12DescriptorHeap* mainDescriptorHeap;
		ID3D12Resource* textureBufferUploadHeap;

		BYTE* imageData;
	};


	
}