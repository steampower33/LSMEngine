#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include <DirectXMath.h>

using namespace DirectX;

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