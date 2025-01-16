#include "Ray.h"

Ray::Ray(float ndcX, float ndcY, XMMATRIX& view, XMMATRIX& proj)
{

	// NDC ��ǥ�� Ŭ�� ������ ��ǥ�� ��ȯ (Z = 0.0f�� Near Plane, Z = 1.0f�� Far Plane)
	XMVECTOR rayNDCNear = XMVectorSet(ndcX, ndcY, 0.0f, 1.0f);
	XMVECTOR rayNDCFar = XMVectorSet(ndcX, ndcY, 1.0f, 1.0f);

	// �� �������� ��Ʈ������ ����Ͽ� ���� ������ ��ǥ�� ��ȯ
	XMMATRIX invProjection = XMMatrixInverse(nullptr, proj);
	XMMATRIX invView = XMMatrixInverse(nullptr, view);

	// Ŭ�� ������ ���� �������� ��ȯ
	XMVECTOR rayViewNear = XMVector3TransformCoord(rayNDCNear, invProjection);
	XMVECTOR rayViewFar = XMVector3TransformCoord(rayNDCFar, invProjection);

	// ���� ������ ���� �������� ��ȯ
	XMVECTOR rayWorldNear = XMVector3TransformCoord(rayViewNear, invView);
	XMVECTOR rayWorldFar = XMVector3TransformCoord(rayViewFar, invView);

	// ������ ������ ���� ���
	XMFLOAT3 originFloat;
	XMStoreFloat3(&originFloat, rayWorldNear);
	XMFLOAT3 farPointFloat;
	XMStoreFloat3(&farPointFloat, rayWorldFar);

	XMFLOAT3 direction = {
		farPointFloat.x - originFloat.x,
		farPointFloat.y - originFloat.y,
		farPointFloat.z - originFloat.z
	};

	// ���� ���� ����ȭ
	XMVECTOR directionVec = XMVector3Normalize(XMLoadFloat3(&direction));

	// ���� ������ ������ ����
	rayOrigin = originFloat;
	XMStoreFloat3(&rayDirection, directionVec);
}

bool Ray::RaySphereIntersect(BoundingSphere& boundingSphere)
{
	// ������ ������ ������ ���ͷ� �ε�
	XMVECTOR origin = XMLoadFloat3(&rayOrigin);
	XMVECTOR direction = XMLoadFloat3(&rayDirection);
	XMVECTOR center = XMLoadFloat3(&boundingSphere.Center);

	// O - C
	XMVECTOR oc = XMVectorSubtract(origin, center);

	// a = D �� D (����ȭ�� ���� �����̶�� a = 1)
	float a = XMVectorGetX(XMVector3Dot(direction, direction));

	// b = 2 * (O - C) �� D
	float b = 2.0f * XMVectorGetX(XMVector3Dot(oc, direction));

	// c = (O - C) �� (O - C) - r^2
	float radius = boundingSphere.Radius;
	float c = XMVectorGetX(XMVector3Dot(oc, oc)) - (radius * radius);

	// �Ǻ��� ���
	float discriminant = b * b - 4.0f * a * c;

	// �Ǻ����� �����̸� �������� ����
	if (discriminant < 0.0f)
		return false;

	// �Ǻ����� 0 �̻��̸� ������
	return true;
}