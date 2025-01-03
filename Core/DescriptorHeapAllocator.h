#pragma once

#include "imgui.h"
#include "d3d12.h"

// Simple free list based allocator
class HeapAllocator
{
public:
	HeapAllocator();
	~HeapAllocator();
	void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap);
	void Destroy();
	void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle);
	void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle);

private:
	ID3D12DescriptorHeap* Heap = nullptr;
	D3D12_DESCRIPTOR_HEAP_TYPE  HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
	D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu = {};
	D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu = {};
	UINT                        HeapHandleIncrement = 0;
	ImVector<int>               FreeIndices;
};