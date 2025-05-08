#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include "DirectXTex.h"
#include <iostream>

#include "Helpers.h"

#include <dxcapi.h>
#include <fstream>
#include <filesystem>

using Microsoft::WRL::ComPtr;

// 참고: DirectX_Graphic-Samples 미니엔진
// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/GraphicsCommon.h

namespace Graphics
{
	// DXC
	extern ComPtr<IDxcCompiler3> compiler;
	extern ComPtr<IDxcUtils> utils;
	extern ComPtr<IDxcIncludeHandler> includeHandler;

	// ROOTSIGNATURE
	extern ComPtr<ID3D12RootSignature> basicRootSignature;
	extern ComPtr<ID3D12RootSignature> blurComputeRootSignature;
	extern ComPtr<ID3D12RootSignature> sphComputeRootSignature;
	extern ComPtr<ID3D12RootSignature> sphRenderRootSignature;
	extern ComPtr<ID3D12RootSignature> sphSSFRSignature;

	// SHADER 
	extern ComPtr<IDxcBlob> basicVS;
	extern ComPtr<IDxcBlob> basicPS;
	extern ComPtr<IDxcBlob> simplePS;

	extern ComPtr<IDxcBlob> normalVS;
	extern ComPtr<IDxcBlob> normalGS;
	extern ComPtr<IDxcBlob> normalPS;

	extern ComPtr<IDxcBlob> skyboxVS;
	extern ComPtr<IDxcBlob> skyboxPS;

	extern ComPtr<IDxcBlob> samplingVS;
	extern ComPtr<IDxcBlob> samplingPS;

	extern ComPtr<IDxcBlob> combineVS;
	extern ComPtr<IDxcBlob> combinePS;

	extern ComPtr<IDxcBlob> samplingCS;
	extern ComPtr<IDxcBlob> blurXCS;
	extern ComPtr<IDxcBlob> blurYCS;

	extern ComPtr<IDxcBlob> depthOnlyVS;
	extern ComPtr<IDxcBlob> depthOnlyPS;

	extern ComPtr<IDxcBlob> postEffectsVS;
	extern ComPtr<IDxcBlob> postEffectsPS;

	extern ComPtr<IDxcBlob> sphClearCountCellCS;
	extern ComPtr<IDxcBlob> sphCountCellCS;
	extern ComPtr<IDxcBlob> sphCellLocalScanCS;
	extern ComPtr<IDxcBlob> sphCellLocalScanBlockCS;
	extern ComPtr<IDxcBlob> sphCellFinalAdditionCS;
	extern ComPtr<IDxcBlob> sphCellScatterCS;
	extern ComPtr<IDxcBlob> sphExternalCS;
	extern ComPtr<IDxcBlob> sphCalcDensityCS;
	extern ComPtr<IDxcBlob> sphCalcPressureForceCS;
	extern ComPtr<IDxcBlob> sphCS;
	extern ComPtr<IDxcBlob> sphVS;
	extern ComPtr<IDxcBlob> sphGS;
	extern ComPtr<IDxcBlob> sphPS;
	extern ComPtr<IDxcBlob> sphSmoothingCS;
	extern ComPtr<IDxcBlob> sphNormalCS;

	extern ComPtr<IDxcBlob> boundsBoxVS;
	extern ComPtr<IDxcBlob> boundsBoxPS;

	// RASTERIZER
	extern D3D12_RASTERIZER_DESC solidRS;
	extern D3D12_RASTERIZER_DESC wireRS;
	extern D3D12_RASTERIZER_DESC solidCCWRS;
	extern D3D12_RASTERIZER_DESC wireCCWRS;
	extern D3D12_RASTERIZER_DESC postProcessingRS;
	extern D3D12_RASTERIZER_DESC depthBiasRS;

	// BLEND
	extern D3D12_BLEND_DESC disabledBlend;
	extern D3D12_BLEND_DESC mirrorBlend;
	extern D3D12_BLEND_DESC accumulateBlend;
	extern D3D12_BLEND_DESC alphaBlend;

	// DEPTH_STENCIL
	extern D3D12_DEPTH_STENCIL_DESC basicDS;
	extern D3D12_DEPTH_STENCIL_DESC disabledDS;
	extern D3D12_DEPTH_STENCIL_DESC maskDS;
	extern D3D12_DEPTH_STENCIL_DESC drawMaskedDS;

	// PSO
	extern ComPtr<ID3D12PipelineState> basicSolidPSO;
	extern ComPtr<ID3D12PipelineState> basicWirePSO;
	extern ComPtr<ID3D12PipelineState> stencilMaskPSO;
	extern ComPtr<ID3D12PipelineState> reflectSolidPSO;
	extern ComPtr<ID3D12PipelineState> reflectWirePSO;
	extern ComPtr<ID3D12PipelineState> skyboxSolidPSO;
	extern ComPtr<ID3D12PipelineState> skyboxWirePSO;
	extern ComPtr<ID3D12PipelineState> skyboxReflectSolidPSO;
	extern ComPtr<ID3D12PipelineState> skyboxReflectWirePSO;
	extern ComPtr<ID3D12PipelineState> mirrorBlendSolidPSO;
	extern ComPtr<ID3D12PipelineState> mirrorBlendWirePSO;

	extern ComPtr<ID3D12PipelineState> normalPSO;
	extern ComPtr<ID3D12PipelineState> samplingPSO;
	extern ComPtr<ID3D12PipelineState> combinePSO;
	extern ComPtr<ID3D12PipelineState> depthOnlyPSO;
	extern ComPtr<ID3D12PipelineState> postEffectsPSO;
	extern ComPtr<ID3D12PipelineState> basicSimplePSPSO;
	extern ComPtr<ID3D12PipelineState> shadowDepthOnlyPSO;

	extern ComPtr<ID3D12PipelineState> samplingCSPSO;
	extern ComPtr<ID3D12PipelineState> blurXCSPSO;
	extern ComPtr<ID3D12PipelineState> blurYCSPSO;

	extern ComPtr<ID3D12PipelineState> sphClearCountCellCSPSO;
	extern ComPtr<ID3D12PipelineState> sphCountCellCSPSO;
	extern ComPtr<ID3D12PipelineState> sphCellLocalScanCSPSO;
	extern ComPtr<ID3D12PipelineState> sphCellLocalScanBlockCSPSO;
	extern ComPtr<ID3D12PipelineState> sphCellFinalAdditionCSPSO;
	extern ComPtr<ID3D12PipelineState> sphCellScatterCSPSO;
	extern ComPtr<ID3D12PipelineState> sphExternalCSPSO;
	extern ComPtr<ID3D12PipelineState> sphCalcDensityCSPSO;
	extern ComPtr<ID3D12PipelineState> sphCalcPressureForceCSPSO;
	extern ComPtr<ID3D12PipelineState> sphCSPSO;
	extern ComPtr<ID3D12PipelineState> sphPSO;
	extern ComPtr<ID3D12PipelineState> sphSmoothingCSPSO;
	extern ComPtr<ID3D12PipelineState> sphNormalCSPSO;

	extern ComPtr<ID3D12PipelineState> boundsBoxPSO;

	// TEXTURE_SIZE
	extern UINT textureSize;
	extern UINT cubeTextureSize;
	extern UINT imguiTextureSize;

	void Initialize(ComPtr<ID3D12Device>& device);
	void InitDXC();
	void InitBasicRootSignature(ComPtr<ID3D12Device>& device);
	void InitSphRenderRootSignature(ComPtr<ID3D12Device>& device);
	void InitPostProcessComputeRootSignature(ComPtr<ID3D12Device>& device);
	void InitSphComputeRootSignature(ComPtr<ID3D12Device>& device);
	void InitSphSSFRSignature(ComPtr<ID3D12Device>& device);
	void InitShaders(ComPtr<ID3D12Device>& device);
	void InitSphShaders(ComPtr<ID3D12Device>& device);
	void InitRasterizerStates();
	void InitBlendStates();
	void InitDepthStencilStates();
	void InitPipelineStates(ComPtr<ID3D12Device>& device);
	void InitSphPipelineStates(ComPtr<ID3D12Device>& device);

	void CreateShader(
		ComPtr<ID3D12Device>& device,
		wstring filename,
		wstring targetProfile, // Shader Target Profile
		ComPtr<IDxcBlob>& shaderBlob);

}