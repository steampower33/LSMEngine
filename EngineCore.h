#pragma once

#include "stdafx.h"
#include <iostream>

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

		void mainloop();
		bool InitD3D();
		void Cleanup();
		void WaitForPreviousFrame();
		bool CreateDevice();
		bool CreateCommandQueue();
		bool CreateSwapChain();
		bool CreateRenderTargetViews();
		bool CreateCommandTools();

	private:
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

		D3D12_COMMAND_QUEUE_DESC cqDesc = {};

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

	};
}