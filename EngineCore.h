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
		~AppBase();

		
		bool RunApplication(HINSTANCE hInstance, int nShowCmd);
		
		// initializeWindow
		bool InitializeWindow(HINSTANCE hInstance, int ShowWnd, bool fullscreen);

		// callback function for windows messages
		LRESULT MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		std::wstring GetLatestWinPixGpuCapturerPath();

		void mainloop();
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

		void Update();
		void UpdatePipeline();
		void Render();

	private:
		struct Vertex {
			Vertex(float x, float y, float z, float u, float v) : pos(x, y, z), texCoord(u, v) {}
			XMFLOAT3 pos;
			XMFLOAT2 texCoord;
		};

		// Handle to the window
		HWND hwnd = NULL;

		// name of the window (not the title)
		LPCTSTR WindowName;

		// title of the window
		LPCTSTR WindowTitle;

		// width and height of the window
		int Width;
		int Height;

		// is window full screen?
		bool FullScreen;

		// we will exit the program when this becomes false
		bool Running;

		// direct3d stuff
		static const int frameBufferCount = 3; // number of buffers we want, 2 for double buffering, 3 for tripple buffering

		IDXGIFactory4* dxgiFactory;
		IDXGIAdapter1* adapter; // adapters are the graphics card (this includes the embedded graphics on the motherboard)
		ID3D12Device* device; // direct3d device

		DXGI_MODE_DESC backBufferDesc = {}; // this is to describe our display mode
		DXGI_SAMPLE_DESC sampleDesc = {}; // describe our multi-sampling. We are not multi-sampling, so we set the count to 1 (we need at least one sample of course)
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		IDXGISwapChain* tempSwapChain;
		IDXGISwapChain3* swapChain; // swapchain used to switch between render targets
		
		ID3D12CommandQueue* commandQueue; // container for command lists
		ID3D12DescriptorHeap* rtvDescriptorHeap; // a descriptor heap to hold resources like the render targets
		ID3D12Resource* renderTargets[frameBufferCount]; // number of render targets equal to buffer count
		ID3D12CommandAllocator* commandAllocator[frameBufferCount]; // we want enough allocators for each buffer * number of threads (we only have one thread)
		ID3D12GraphicsCommandList* commandList; // a command list we can record commands into, then execute them to render the frame
		ID3D12Fence* fence[frameBufferCount];    // an object that is locked while our command list is being executed by the gpu. We need as many 
		//as we have allocators (more if we want to know when the gpu is finished with an asset)

		HANDLE fenceEvent; // a handle to an event when our fence is unlocked by the gpu
		UINT64 fenceValue[frameBufferCount]; // this value is incremented each frame. each fence will have its own value
		
		int frameIndex; // current rtv we are on
		int rtvDescriptorSize; // size of the rtv descriptor on the device (all front and back buffers will be the same size)
		
		ID3D12RootSignature* rootSignature; // root signature defines data shaders will access

		ID3DBlob* errorBuff; // a buffer holding the error data if any

		ID3DBlob* vertexShader; // d3d blob for holding vertex shader bytecode
		D3D12_SHADER_BYTECODE vertexShaderBytecode = {};

		ID3DBlob* pixelShader;
		D3D12_SHADER_BYTECODE pixelShaderBytecode = {};

		ID3D12PipelineState* pipelineStateObject; // pso containing a pipeline state
		
		ID3D12Resource* vertexBuffer; // a default buffer in GPU memory that we will load vertex data for our triangle into
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView; // a structure containing a pointer to the vertex data in gpu memory
		// the total size of the buffer, and the size of each element (vertex)

		ID3D12Resource* indexBuffer; // a default buffer in GPU memory that we will load index data for our triangle into
		D3D12_INDEX_BUFFER_VIEW indexBufferView; // a structure holding information about the index buffer

		ID3D12Resource* depthStencilBuffer; // This is the memory for our depth buffer. it will also be used for a stencil buffer in a later tutorial
		ID3D12DescriptorHeap* dsDescriptorHeap; // This is a heap for our depth/stencil buffer descriptor

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

		ID3D12Resource* constantBufferUploadHeaps[frameBufferCount]; // this is the memory on the gpu where constant buffers for each frame will be placed

		UINT8* cbvGPUAddress[frameBufferCount]; // this is a pointer to each of the constant buffer resource heaps

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