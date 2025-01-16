#include "Ray.h"

Ray::Ray(float ndcX, float ndcY, XMMATRIX& view, XMMATRIX& proj)
{

	// NDC 좌표를 클립 공간의 좌표로 변환 (Z = 0.0f는 Near Plane, Z = 1.0f는 Far Plane)
	XMVECTOR rayNDCNear = XMVectorSet(ndcX, ndcY, 0.0f, 1.0f);
	XMVECTOR rayNDCFar = XMVectorSet(ndcX, ndcY, 1.0f, 1.0f);

	// 역 프로젝션 매트릭스를 사용하여 월드 공간의 좌표로 변환
	XMMATRIX invProjection = XMMatrixInverse(nullptr, proj);
	XMMATRIX invView = XMMatrixInverse(nullptr, view);

	// 클립 공간을 보기 공간으로 변환
	XMVECTOR rayViewNear = XMVector3TransformCoord(rayNDCNear, invProjection);
	XMVECTOR rayViewFar = XMVector3TransformCoord(rayNDCFar, invProjection);

	// 보기 공간을 월드 공간으로 변환
	XMVECTOR rayWorldNear = XMVector3TransformCoord(rayViewNear, invView);
	XMVECTOR rayWorldFar = XMVector3TransformCoord(rayViewFar, invView);

	// 광선의 원점과 방향 계산
	XMFLOAT3 originFloat;
	XMStoreFloat3(&originFloat, rayWorldNear);
	XMFLOAT3 farPointFloat;
	XMStoreFloat3(&farPointFloat, rayWorldFar);

	XMFLOAT3 direction = {
		farPointFloat.x - originFloat.x,
		farPointFloat.y - originFloat.y,
		farPointFloat.z - originFloat.z
	};

	// 방향 벡터 정규화
	XMVECTOR directionVec = XMVector3Normalize(XMLoadFloat3(&direction));

	// 최종 광선의 원점과 방향
	rayOrigin = originFloat;
	XMStoreFloat3(&rayDirection, directionVec);
}

bool Ray::RaySphereIntersect(BoundingSphere& boundingSphere)
{
	// 광선의 원점과 방향을 벡터로 로드
	XMVECTOR origin = XMLoadFloat3(&rayOrigin);
	XMVECTOR direction = XMLoadFloat3(&rayDirection);
	XMVECTOR center = XMLoadFloat3(&boundingSphere.Center);

	// O - C
	XMVECTOR oc = XMVectorSubtract(origin, center);

	// a = D · D (정규화된 광선 방향이라면 a = 1)
	float a = XMVectorGetX(XMVector3Dot(direction, direction));

	// b = 2 * (O - C) · D
	float b = 2.0f * XMVectorGetX(XMVector3Dot(oc, direction));

	// c = (O - C) · (O - C) - r^2
	float radius = boundingSphere.Radius;
	float c = XMVectorGetX(XMVector3Dot(oc, oc)) - (radius * radius);

	// 판별식 계산
	float discriminant = b * b - 4.0f * a * c;

	// 판별식이 음수이면 교차하지 않음
	if (discriminant < 0.0f)
		return false;

	// 판별식이 0 이상이면 교차함
	return true;
}