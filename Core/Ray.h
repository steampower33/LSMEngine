#pragma once

#include <DirectXMath.h>
#include <DirectXCollision.h>

using namespace DirectX;

class Ray
{
public:
	Ray(float ndcX, float ndcY, XMMATRIX& view, XMMATRIX& proj);
	~Ray() {}

	bool RaySphereIntersect(BoundingSphere& boundingSphere);

private:
	XMFLOAT3 rayOrigin;
	XMFLOAT3 rayDirection;
};