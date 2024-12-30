#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"
#include "DirectXTex.h"

#include <iostream>
#include <string>
#include <wrl.h>
#include <shellapi.h>
#include <algorithm>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include <stdexcept>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

inline std::string HrToString(HRESULT hr)
{
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
}

class HrException : public std::runtime_error
{
public:
    HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
    HRESULT Error() const { return m_hr; }
private:
    const HRESULT m_hr;
};

#define SAFE_RELEASE(p) if (p) (p)->Release()

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw HrException(hr);
    }
}

__declspec(align(256)) struct GlobalConstants
{
    XMFLOAT4X4 view;
    XMFLOAT4X4 proj;
    XMFLOAT4X4 dummy1;
    XMFLOAT4X4 dummy2;
};

// 주로 Vertex/Geometry 쉐이더에서 사용
__declspec(align(256)) struct MeshConstants {
    XMFLOAT4X4 world;
    XMFLOAT4X4 worldIT;
    XMFLOAT4X4 dummy1;
    XMFLOAT4X4 dummy2;
};