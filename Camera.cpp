#include "Camera.h"

namespace EngineCore
{
	Camera::Camera() :
		m_initialPosition(0, 0, -2),
		m_position(m_initialPosition),
		m_lookDirection(0, 0, 1),
		m_upDirection(0, 1, 0)
	{

	}

	XMMATRIX Camera::GetViewMatrix()
	{
		return XMMatrixLookToLH(XMLoadFloat3(&m_position), XMLoadFloat3(&m_lookDirection), XMLoadFloat3(&m_upDirection));
	}

	XMMATRIX Camera::GetProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane)
	{
		return XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);
	}
}