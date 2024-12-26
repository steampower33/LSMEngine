#include "Camera.h"

namespace EngineCore
{
	Camera::Camera() :
		m_initialPosition(0, 0, -2),
		m_position(m_initialPosition),
		m_lookDirection(0, 0, 1),
		m_upDirection(0, 1, 0),
		m_speed(3.0f)
	{

	}

	void Camera::Update(float dt)
	{
		XMFLOAT3 move(0, 0, 0);

		if (m_keysPressed.w) move.z += 1.0f;
		if (m_keysPressed.s) move.z -= 1.0f;
		if (m_keysPressed.a) move.x -= 1.0f;
		if (m_keysPressed.d) move.x += 1.0f;
		if (m_keysPressed.q) move.y += 1.0f;
		if (m_keysPressed.e) move.y -= 1.0f;

		if (move.x != 0.0f || move.y != 0.0f || move.z != 0.0f)
		{
			XMVECTOR v = XMVector3Normalize(XMLoadFloat3(&move)) * m_speed * dt;

			XMStoreFloat3(&m_position, XMLoadFloat3(&m_position) + v);
		}
	}

	void Camera::KeyDown(WPARAM key)
	{
		switch (key)
		{
		case 'W':
			m_keysPressed.w = true;
			break;
		case 'A':
			m_keysPressed.a = true;
			break;
		case 'S':
			m_keysPressed.s = true;
			break;
		case 'D':
			m_keysPressed.d = true;
			break;
		case 'Q':
			m_keysPressed.q = true;
			break;
		case 'E':
			m_keysPressed.e = true;
			break;
		case VK_LEFT:
			m_keysPressed.left = true;
			break;
		case VK_RIGHT:
			m_keysPressed.right = true;
			break;
		case VK_UP:
			m_keysPressed.up = true;
			break;
		case VK_DOWN:
			m_keysPressed.down = true;
			break;
		}
	}

	void Camera::KeyUp(WPARAM key)
	{
		switch (key)
		{
		case 'W':
			m_keysPressed.w = false;
			break;
		case 'A':
			m_keysPressed.a = false;
			break;
		case 'S':
			m_keysPressed.s = false;
			break;
		case 'D':
			m_keysPressed.d = false;
			break;
		case 'Q':
			m_keysPressed.q = false;
			break;
		case 'E':
			m_keysPressed.e = false;
			break;
		case VK_LEFT:
			m_keysPressed.left = false;
			break;
		case VK_RIGHT:
			m_keysPressed.right = false;
			break;
		case VK_UP:
			m_keysPressed.up = false;
			break;
		case VK_DOWN:
			m_keysPressed.down = false;
			break;
		}
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