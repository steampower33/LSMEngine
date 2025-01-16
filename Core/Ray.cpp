#include "Ray.h"

Ray::Ray(XMFLOAT3& originFloat, XMVECTOR& directionVec)
{
	// 최종 광선의 원점과 방향
	rayOrigin = originFloat;
	XMStoreFloat3(&rayDirection, directionVec);
}

bool Ray::RaySphereIntersect(shared_ptr<BoundingSphere>& boundingSphere, float& dist)
{
	// 광선의 원점과 방향을 벡터로 로드
	XMVECTOR origin = XMLoadFloat3(&rayOrigin);
	XMVECTOR direction = XMLoadFloat3(&rayDirection);
	XMVECTOR center = XMLoadFloat3(&boundingSphere->Center);

	// O - C
	XMVECTOR oc = XMVectorSubtract(origin, center);

	// a = D · D (정규화된 광선 방향이라면 a = 1)
	float a = XMVectorGetX(XMVector3Dot(direction, direction));

	// b = 2 * (O - C) · D
	float b = 2.0f * XMVectorGetX(XMVector3Dot(oc, direction));

	// c = (O - C) · (O - C) - r^2
	float radius = boundingSphere->Radius;
	float c = XMVectorGetX(XMVector3Dot(oc, oc)) - (radius * radius);

	// 판별식 계산
	float discriminant = b * b - 4.0f * a * c;

	// 판별식이 음수이면 교차하지 않음
	if (discriminant < 0.0f)
		return false;

	float sqrtDiscriminant = sqrtf(discriminant);
	float t0 = (-b - sqrtDiscriminant) / (2.0f * a);
	float t1 = (-b + sqrtDiscriminant) / (2.0f * a);

	// 가장 작은 양의 t 값을 선택
	if (t0 >= 0.0f && t0 <= t1)
	{
		dist = t0;
		return true;
	}
	if (t1 >= 0.0f)
	{
		dist = t1;
		return true;
	}

	return false;
}