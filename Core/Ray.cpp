#include "Ray.h"

Ray::Ray(XMFLOAT3& originFloat, XMVECTOR& directionVec)
{
	// ���� ������ ������ ����
	rayOrigin = originFloat;
	XMStoreFloat3(&rayDirection, directionVec);
}

bool Ray::RaySphereIntersect(shared_ptr<BoundingSphere>& boundingSphere, float& dist)
{
	// ������ ������ ������ ���ͷ� �ε�
	XMVECTOR origin = XMLoadFloat3(&rayOrigin);
	XMVECTOR direction = XMLoadFloat3(&rayDirection);
	XMVECTOR center = XMLoadFloat3(&boundingSphere->Center);

	// O - C
	XMVECTOR oc = XMVectorSubtract(origin, center);

	// a = D �� D (����ȭ�� ���� �����̶�� a = 1)
	float a = XMVectorGetX(XMVector3Dot(direction, direction));

	// b = 2 * (O - C) �� D
	float b = 2.0f * XMVectorGetX(XMVector3Dot(oc, direction));

	// c = (O - C) �� (O - C) - r^2
	float radius = boundingSphere->Radius;
	float c = XMVectorGetX(XMVector3Dot(oc, oc)) - (radius * radius);

	// �Ǻ��� ���
	float discriminant = b * b - 4.0f * a * c;

	// �Ǻ����� �����̸� �������� ����
	if (discriminant < 0.0f)
		return false;

	float sqrtDiscriminant = sqrtf(discriminant);
	float t0 = (-b - sqrtDiscriminant) / (2.0f * a);
	float t1 = (-b + sqrtDiscriminant) / (2.0f * a);

	// ���� ���� ���� t ���� ����
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