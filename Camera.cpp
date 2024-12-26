#include "Camera.h"

namespace EngineCore
{
	using namespace std;

	Camera::Camera() :
		m_initPos(0, 0, -2),
		m_pos(m_initPos),
		m_lookDir(0, 0, 1),
		m_upDir(0, 1, 0),
		m_rightDir(1, 0, 0),
		m_speed(3.0f)
	{

	}

	void Camera::Update(float dt)
	{
		if (m_keysPressed.w)
			MoveForward(dt);
		if (m_keysPressed.s)
			MoveForward(-dt);
		if (m_keysPressed.a)
			MoveRight(-dt);
		if (m_keysPressed.d)
			MoveRight(dt);
		if (m_keysPressed.q)
			MoveUp(dt);
		if (m_keysPressed.e)
			MoveUp(-dt);
	}

	void Camera::MoveForward(float dt) {
		// 이동후의_위치 = 현재_위치 + 이동방향 * 속도 * 시간차이;
		XMStoreFloat3(&m_pos, XMLoadFloat3(&m_pos) + XMLoadFloat3(&m_lookDir) * m_speed * dt);
	}

	void Camera::MoveUp(float dt) {
		// 이동후의_위치 = 현재_위치 + 이동방향 * 속도 * 시간차이;
		XMStoreFloat3(&m_pos, XMLoadFloat3(&m_pos) + XMLoadFloat3(&m_upDir) * m_speed * dt);
	}

	void Camera::MoveRight(float dt) { 
		XMStoreFloat3(&m_pos, XMLoadFloat3(&m_pos) + XMLoadFloat3(&m_rightDir) * m_speed * dt);
	}


	void Camera::UpdateMouse(float mouseX, float mouseY, float m_screenWidth, float m_screenHeight)
	{
		m_cursorNdcX = mouseX * 2.0f / m_screenWidth - 1.0f;
		m_cursorNdcY = -mouseY * 2.0f / m_screenHeight + 1.0f;

		m_cursorNdcX = std::clamp(m_cursorNdcX, -1.0f, 1.0f);
		m_cursorNdcY = std::clamp(m_cursorNdcY, -1.0f, 1.0f);

		m_yaw = m_cursorNdcX * DirectX::XM_2PI;     // 좌우 360도
		m_pitch = m_cursorNdcY * DirectX::XM_PIDIV2; // 위 아래 90도

		XMStoreFloat3(&m_lookDir, XMVector3TransformCoord(XMVECTOR{ 0.0f, 0.0f, 1.0f }, XMMatrixRotationY(m_yaw)));
		XMStoreFloat3(&m_rightDir, XMVector3Cross(XMLoadFloat3(&m_upDir), XMLoadFloat3(&m_lookDir)));
	}

	XMMATRIX Camera::GetViewMatrix()
	{
		XMVECTOR pos = XMLoadFloat3(&m_pos);

		return XMMatrixTranslationFromVector(-pos) * XMMatrixRotationY(-m_yaw) * XMMatrixRotationX(m_pitch);
	}

	XMMATRIX Camera::GetProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane)
	{
		return XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);
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
		}
	}

}