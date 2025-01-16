#pragma once

#include <iostream>
#include <DirectXMath.h>
#include <DirectXCollision.h>

using namespace std;
using namespace DirectX;

class Ray
{
public:
	Ray(XMFLOAT3& originFloat, XMVECTOR& directionVec);
	~Ray() {}

	bool RaySphereIntersect(shared_ptr<BoundingSphere>& boundingSphere, float& dist);

private:
	XMFLOAT3 rayOrigin;
	XMFLOAT3 rayDirection;
};