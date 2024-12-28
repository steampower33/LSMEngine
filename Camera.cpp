#include "Camera.h"

namespace EngineCore
{
	using namespace std;

	Camera::Camera() :
		m_initPos(-5.0f, 0.0f, -5.0f),
		m_pos(m_initPos),
		m_lookDir(0.0f, 0.0f, 1.0f),
		m_upDir(0.0f, 1.0f, 0.0f),
		m_rightDir(1.0f, 0.0f, 0.0f),
		m_cursorNdcX(0.0f),
		m_cursorNdcY(0.0f),
		m_yaw(0.7854f),
		m_pitch(0.0f),
		m_speed(5.0f),
		m_useFirstPersonView(false)
	{

	}

	void Camera::Update(float deltaX, float deltaY, float dt, bool &isMouseMove)
	{
		if (m_useFirstPersonView) {
			if (m_keysPressed.w)
				MoveForward(dt);
			if (m_keysPressed.s)
				MoveForward(-dt);
			if (m_keysPressed.a)
				MoveRight(-dt);
			if (m_keysPressed.d)
				MoveRight(dt);
			if (m_keysPressed.q)
				MoveUp(-dt);
			if (m_keysPressed.e)
				MoveUp(dt);

			if (isMouseMove)
			{
				isMouseMove = false;
				UpdateMouse(deltaX, deltaY, dt);
			}
		}

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


	void Camera::UpdateMouse(float deltaX, float deltaY, float dt)
	{
		if (m_useFirstPersonView) {
			// 마우스 이동량(Delta)에 속도와 deltaTime 적용
			m_yaw += deltaX * m_speed * dt;
			m_pitch += deltaY * m_speed * dt;

			// Pitch 제한 (카메라가 위/아래로 90도 이상 회전하지 않도록)
			m_pitch = std::clamp(m_pitch, -DirectX::XM_PIDIV2, DirectX::XM_PIDIV2);

			// 새로운 방향 벡터 계산
			XMVECTOR forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // 카메라의 기본 전방 방향
			XMMATRIX yawMatrix = XMMatrixRotationY(m_yaw);          // Yaw 회전 행렬
			XMMATRIX pitchMatrix = XMMatrixRotationX(m_pitch);      // Pitch 회전 행렬

			XMVECTOR lookDir = XMVector3TransformNormal(forward, pitchMatrix * yawMatrix);
			XMStoreFloat3(&m_lookDir, lookDir);

			// 카메라의 오른쪽 방향도 업데이트
			XMVECTOR rightDir = XMVector3Cross(XMLoadFloat3(&m_upDir), lookDir);
			XMStoreFloat3(&m_rightDir, rightDir);
		}
	}

	XMMATRIX Camera::GetViewMatrix()
	{
		XMVECTOR pos = XMLoadFloat3(&m_pos);

		return XMMatrixTranslationFromVector(-pos) * XMMatrixRotationY(-m_yaw) * XMMatrixRotationX(-m_pitch);
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