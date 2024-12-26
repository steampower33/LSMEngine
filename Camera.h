#pragma once

#include <DirectXMath.h>

namespace EngineCore
{
	using namespace DirectX;

	class Camera
	{
	public:
		Camera();
		XMMATRIX GetViewMatrix();
		XMMATRIX GetProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane);

	private:
		// set starting camera state
		XMFLOAT3 m_initialPosition;
		XMFLOAT3 m_position;
		XMFLOAT3 m_lookDirection;
		XMFLOAT3 m_upDirection;

	};
}