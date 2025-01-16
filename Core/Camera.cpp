#include "Camera.h"

using namespace std;

Camera::Camera(float aspectRatio) : 
	m_aspectRatio(aspectRatio),
	m_pos(0.0f, 0.0f, -5.0f),
	m_lookDir(0.0f, 0.0f, 1.0f),
	m_upDir(0.0f, 1.0f, 0.0f),
	m_rightDir(1.0f, 0.0f, 0.0f),
	m_cursorNdcX(0.0f),
	m_cursorNdcY(0.0f),
	m_yaw(0.0f),
	m_pitch(0.0f),
	m_moveSpeed(10.0f),
	m_mouseSensitivity(5.0f),
	m_useFirstPersonView(false)
{
}

void Camera::Update(float deltaX, float deltaY, float dt, bool& isMouseMove)
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

		//LogCameraState();
	}

}

void Camera::LogCameraState()
{
	std::cout << "Camera Position: ("
		<< m_pos.x << "f, "
		<< m_pos.y << "f, "
		<< m_pos.z << "f)" << std::endl;
	std::cout << "Yaw: " << m_yaw << "f, Pitch: " << m_pitch << "f" << std::endl;
	std::cout << "Look Direction: ("
		<< m_lookDir.x << "f, "
		<< m_lookDir.y << "f, "
		<< m_lookDir.z << "f)" << std::endl;
	std::cout << "Up Direction: ("
		<< m_lookDir.x << "f, "
		<< m_lookDir.y << "f, "
		<< m_lookDir.z << "f)" << std::endl;
	std::cout << "Right Direction: ("
		<< m_rightDir.x << "f, "
		<< m_rightDir.y << "f, "
		<< m_rightDir.z << "f)" << std::endl;
}

void Camera::MoveForward(float dt) {
	// �̵�����_��ġ = ����_��ġ + �̵����� * �ӵ� * �ð�����;
	XMStoreFloat3(&m_pos, XMLoadFloat3(&m_pos) + XMLoadFloat3(&m_lookDir) * m_moveSpeed * dt);
}

void Camera::MoveUp(float dt) {
	// �̵�����_��ġ = ����_��ġ + �̵����� * �ӵ� * �ð�����;
	XMStoreFloat3(&m_pos, XMLoadFloat3(&m_pos) + XMLoadFloat3(&m_upDir) * m_moveSpeed * dt);
}

void Camera::MoveRight(float dt) {
	XMStoreFloat3(&m_pos, XMLoadFloat3(&m_pos) + XMLoadFloat3(&m_rightDir) * m_moveSpeed * dt);
}


void Camera::UpdateMouse(float deltaX, float deltaY, float dt)
{
	// ���콺 �̵���(Delta)�� �ӵ��� deltaTime ����
	m_yaw += deltaX * m_mouseSensitivity * dt;
	m_pitch += deltaY * m_mouseSensitivity * dt;

	// Pitch ���� (ī�޶� ��/�Ʒ��� 90�� �̻� ȸ������ �ʵ���)
	m_pitch = std::clamp(m_pitch, -DirectX::XM_PIDIV2, DirectX::XM_PIDIV2);

	// ���ο� ���� ���� ���
	XMVECTOR forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // ī�޶��� �⺻ ���� ����
	XMMATRIX yawMatrix = XMMatrixRotationY(m_yaw);          // Yaw ȸ�� ���
	XMMATRIX pitchMatrix = XMMatrixRotationX(m_pitch);      // Pitch ȸ�� ���

	XMVECTOR lookDir = XMVector3TransformNormal(forward, pitchMatrix * yawMatrix);
	lookDir = XMVector3Normalize(lookDir); // ����ȭ �߰�
	XMStoreFloat3(&m_lookDir, lookDir);

	// ī�޶��� ������ ���⵵ ������Ʈ
	XMVECTOR upDir = XMLoadFloat3(&m_upDir); // upDir�� �ε��Ͽ� ��Ȯ�� ����
	XMVECTOR rightDir = XMVector3Cross(upDir, lookDir);
	rightDir = XMVector3Normalize(rightDir); // ����ȭ �߰�
	XMStoreFloat3(&m_rightDir, rightDir);
}

XMMATRIX Camera::GetViewMatrix()
{
	XMVECTOR pos = XMLoadFloat3(&m_pos);                      // ī�޶� ��ġ
	XMVECTOR target = XMVectorAdd(pos, XMLoadFloat3(&m_lookDir)); // ī�޶� �ٶ󺸴� ����
	XMVECTOR up = XMLoadFloat3(&m_upDir);                      // �� ����

	return XMMatrixLookAtLH(pos, target, up);
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
