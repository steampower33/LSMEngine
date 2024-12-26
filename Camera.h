#pragma once

#include <d3d12.h>
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
		
		void KeyDown(WPARAM key);
		void KeyUp(WPARAM key);
		void Update(float dt);

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
		XMFLOAT3 m_initialPosition;
		XMFLOAT3 m_position;
		XMFLOAT3 m_lookDirection;
		XMFLOAT3 m_upDirection;

		float m_speed;

		KeysPressed m_keysPressed;
	};
}