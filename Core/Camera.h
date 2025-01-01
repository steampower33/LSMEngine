#pragma once

#include <iostream>
#include <d3d12.h>
#include <algorithm>
#include <DirectXMath.h>

using namespace DirectX;

class Camera
{
public:
	Camera();
	XMMATRIX GetViewMatrix();
	XMMATRIX GetProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane);

	void KeyDown(WPARAM key);
	void KeyUp(WPARAM key);
	void Update(float deltaX, float deltaY, float dt, bool& isMouseMove);
	void UpdateMouse(float deltaX, float deltaY, float dt);
	void MoveForward(float dt);
	void MoveRight(float dt);
	void MoveUp(float dt);

	void LogCameraState();

	bool m_useFirstPersonView;

private:
	struct KeysPressed
	{
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

	// set starting camera state
	XMFLOAT3 m_initPos;
	XMFLOAT3 m_pos;
	XMFLOAT3 m_lookDir;
	XMFLOAT3 m_upDir;
	XMFLOAT3 m_rightDir;

	// 마우스 커서 위치 저장 (Picking에 사용)
	float m_cursorNdcX;
	float m_cursorNdcY;

	// roll, pitch, yaw
	// https://en.wikipedia.org/wiki/Aircraft_principal_axes
	float m_yaw;
	float m_pitch;

	float m_moveSpeed;
	float m_mouseSensitivity;

	KeysPressed m_keysPressed;
};