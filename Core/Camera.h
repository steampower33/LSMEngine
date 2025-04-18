#pragma once

#include <iostream>
#include <d3d12.h>
#include <algorithm>
#include <DirectXMath.h>

using namespace DirectX;

struct KeysPressed
{
	KeysPressed() :
		w(false), a(false), s(false), d(false),
		q(false), e(false),
		left(false), right(false), up(false), down(false) {
	}

	bool w;
	bool a;
	bool s;
	bool d;
	bool q;
	bool e;

	bool left;
	bool right;
	bool up;
	bool down;
};

class Camera
{
public:
	Camera(float aspectRatio);
	XMFLOAT3 GetEyePos();
	XMMATRIX GetViewMatrix();
	XMMATRIX GetProjectionMatrix();

	void KeyDown(WPARAM key);
	void KeyUp(WPARAM key);
	void Update(float deltaX, float deltaY, float dt, bool& isMouseMove);
	void UpdateMouse(float deltaX, float deltaY, float dt);
	void MoveForward(float dt);
	void MoveRight(float dt);
	void MoveUp(float dt);

	void LogCameraState();

	bool m_useFirstPersonView = false;

	XMFLOAT3 m_pos{ 0.0f, 0.0f, -16.0f };

	// roll, pitch, yaw
	// https://en.wikipedia.org/wiki/Aircraft_principal_axes
	float m_yaw = 0.0f;
	float m_pitch = 0.0f;

private:
	XMFLOAT3 m_lookDir{ 0.0f, 0.0f, 1.0f };
	XMFLOAT3 m_upDir{ 0.0f, 1.0f, 0.0f };
	XMFLOAT3 m_rightDir{ 1.0f, 0.0f, 0.0f };

	// 마우스 커서 위치 저장 (Picking에 사용)
	float m_cursorNdcX = 0.0f;
	float m_cursorNdcY = 0.0f;

	float m_moveSpeed = 10.0f;
	float m_mouseSensitivity = 5.0f;

	KeysPressed m_keysPressed;

	bool m_isKeyMove = false;

	float m_projFovAngleY = 90.0f;
	float m_nearZ = 0.01f;
	float m_farZ = 100.0f;
	float m_aspectRatio;
};