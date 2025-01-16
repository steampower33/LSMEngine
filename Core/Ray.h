#pragma once

#include <DirectXMath.h>
#include <DirectXCollision.h>

using namespace DirectX;

class Ray
{
public:
	Ray(XMFLOAT3& originFloat, XMVECTOR& directionVec);
	~Ray() {}

	bool RaySphereIntersect(BoundingSphere& boundingSphere, float& dist);

private:
	XMFLOAT3 rayOrigin;
	XMFLOAT3 rayDirection;
};