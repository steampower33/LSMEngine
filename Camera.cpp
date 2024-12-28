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
		// �̵�����_��ġ = ����_��ġ + �̵����� * �ӵ� * �ð�����;
		XMStoreFloat3(&m_pos, XMLoadFloat3(&m_pos) + XMLoadFloat3(&m_lookDir) * m_speed * dt);
	}

	void Camera::MoveUp(float dt) {
		// �̵�����_��ġ = ����_��ġ + �̵����� * �ӵ� * �ð�����;
		XMStoreFloat3(&m_pos, XMLoadFloat3(&m_pos) + XMLoadFloat3(&m_upDir) * m_speed * dt);
	}

	void Camera::MoveRight(float dt) {
		XMStoreFloat3(&m_pos, XMLoadFloat3(&m_pos) + XMLoadFloat3(&m_rightDir) * m_speed * dt);
	}


	void Camera::UpdateMouse(float deltaX, float deltaY, float dt)
	{
		if (m_useFirstPersonView) {
			// ���콺 �̵���(Delta)�� �ӵ��� deltaTime ����
			m_yaw += deltaX * m_speed * dt;
			m_pitch += deltaY * m_speed * dt;

			// Pitch ���� (ī�޶� ��/�Ʒ��� 90�� �̻� ȸ������ �ʵ���)
			m_pitch = std::clamp(m_pitch, -DirectX::XM_PIDIV2, DirectX::XM_PIDIV2);

			// ���ο� ���� ���� ���
			XMVECTOR forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // ī�޶��� �⺻ ���� ����
			XMMATRIX yawMatrix = XMMatrixRotationY(m_yaw);          // Yaw ȸ�� ���
			XMMATRIX pitchMatrix = XMMatrixRotationX(m_pitch);      // Pitch ȸ�� ���

			XMVECTOR lookDir = XMVector3TransformNormal(forward, pitchMatrix * yawMatrix);
			XMStoreFloat3(&m_lookDir, lookDir);

			// ī�޶��� ������ ���⵵ ������Ʈ
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