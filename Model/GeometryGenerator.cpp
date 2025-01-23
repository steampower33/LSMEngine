#include "GeometryGenerator.h"

vector<MeshData> GeometryGenerator::ReadFromFile(string basePath, string filename)
{

	ModelLoader modelLoader;
	modelLoader.Load(basePath, filename);

	vector<MeshData>& meshes = modelLoader.meshes;

	// Normalize vertices
	XMFLOAT3 vmin(1000, 1000, 1000);
	XMFLOAT3 vmax(-1000, -1000, -1000);
	for (auto& mesh : meshes) {
		for (auto& v : mesh.vertices) {
			vmin.x = XMMin(vmin.x, v.position.x);
			vmin.y = XMMin(vmin.y, v.position.y);
			vmin.z = XMMin(vmin.z, v.position.z);
			vmax.x = XMMax(vmax.x, v.position.x);
			vmax.y = XMMax(vmax.y, v.position.y);
			vmax.z = XMMax(vmax.z, v.position.z);
		}
	}

	float dx = vmax.x - vmin.x, dy = vmax.y - vmin.y, dz = vmax.z - vmin.z;
	float dl = XMMax(XMMax(dx, dy), dz);
	float cx = (vmax.x + vmin.x) * 0.5f, cy = (vmax.y + vmin.y) * 0.5f,
		cz = (vmax.z + vmin.z) * 0.5f;

	for (auto& mesh : meshes) {
		for (auto& v : mesh.vertices) {
			v.position.x = (v.position.x - cx) / dl;
			v.position.y = (v.position.y - cy) / dl;
			v.position.z = (v.position.z - cz) / dl;
		}
	}

	for (auto& mesh : meshes)
	{
		CalculateTangents(mesh);
	}

	return meshes;
}

MeshData GeometryGenerator::MakeSquare(const float scale) {

	MeshData meshData;

	meshData.vertices =
	{
		// front
		{ -scale, -scale, 0.0f, 0.0f, scale, 0.0f, 0.0f, -1.0f },
		{ -scale,  scale, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f },
		{  scale,  scale, 0.0f, scale, 0.0f, 0.0f, 0.0f, -1.0f },
		{  scale, -scale, 0.0f, scale, scale, 0.0f, 0.0f, -1.0f },
	};

	meshData.indices = {
			0, 1, 2, 0, 2, 3,
	};

	CalculateTangents(meshData);

	return meshData;
}

MeshData GeometryGenerator::MakeBox(const float scale) {
	MeshData meshData;

	meshData.vertices =
	{
		// front
		{ -scale, -scale, -scale, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f },
		{ -scale,  scale, -scale, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f },
		{  scale,  scale, -scale, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f },
		{  scale, -scale, -scale, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f },

		// back
		{  scale, -scale,  scale, 0.0f, 1.0f, 0.0f, 0.0f,  1.0f },
		{  scale,  scale,  scale, 0.0f, 0.0f, 0.0f, 0.0f,  1.0f },
		{ -scale,  scale,  scale, 1.0f, 0.0f, 0.0f, 0.0f,  1.0f },
		{ -scale, -scale,  scale, 1.0f, 1.0f, 0.0f, 0.0f,  1.0f },

		// top
		{ -scale,  scale, -scale, 0.0f, 1.0f, 0.0f,  1.0f, 0.0f },
		{ -scale,  scale,  scale, 0.0f, 0.0f, 0.0f,  1.0f, 0.0f },
		{  scale,  scale,  scale, 1.0f, 0.0f, 0.0f,  1.0f, 0.0f },
		{  scale,  scale, -scale, 1.0f, 1.0f, 0.0f,  1.0f, 0.0f },

		// bottom
		{ -scale, -scale,  scale, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f },
		{ -scale, -scale, -scale, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f },
		{  scale, -scale, -scale, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f },
		{  scale, -scale,  scale, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f },

		// left
		{ -scale, -scale,  scale, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f },
		{ -scale,  scale,  scale, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f },
		{ -scale,  scale, -scale, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f },
		{ -scale, -scale, -scale, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f },

		// right
		{  scale, -scale, -scale, 0.0f, 1.0f,  1.0f, 0.0f, 0.0f },
		{  scale,  scale, -scale, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f },
		{  scale,  scale,  scale, 1.0f, 0.0f,  1.0f, 0.0f, 0.0f },
		{  scale, -scale,  scale, 1.0f, 1.0f,  1.0f, 0.0f, 0.0f },
	};

	meshData.indices = {
			0, 1, 2, 0, 2, 3,
			4, 5, 6, 4, 6, 7,
			8, 9, 10, 8, 10, 11,
			12, 13, 14, 12, 14, 15,
			16, 17, 18, 16, 18, 19,
			20, 21, 22, 20, 22, 23,
	};

	CalculateTangents(meshData);

	return meshData;
}

MeshData GeometryGenerator::MakeCylinder(
	const float bottomRadius, const float topRadius,
	float height, int sliceCount) {

	const float dTheta = -XM_2PI / float(sliceCount);

	MeshData meshData;

	vector<Vertex>& vertices = meshData.vertices;

	XMVECTOR bottomPosition = { bottomRadius, -0.5f * height, 0.0f };

	for (int i = 0; i <= sliceCount; i++) {
		Vertex v;

		XMMATRIX rotationY = XMMatrixRotationY(dTheta * float(i));
		XMVECTOR position = XMVector3TransformCoord(bottomPosition, rotationY);

		XMStoreFloat3(&v.position, position);

		XMMATRIX translationY = XMMatrixTranslation(0.0f, -v.position.y, 0.0f);
		XMVECTOR normal = XMVector3TransformCoord(position, translationY);

		XMStoreFloat3(&v.normal, XMVector3Normalize(normal));

		v.texcoord = { float(i) / sliceCount, 1.0f };

		vertices.push_back(v);
	}


	XMVECTOR upPosition = { topRadius, 0.5f * height, 0.0f };

	for (int i = 0; i <= sliceCount; i++) {
		Vertex v;

		XMMATRIX rotationY = XMMatrixRotationY(dTheta * float(i));
		XMVECTOR position = XMVector3TransformCoord(upPosition, rotationY);

		XMStoreFloat3(&v.position, position);

		XMMATRIX translationY = XMMatrixTranslation(0.0f, -v.position.y, 0.0f);
		XMVECTOR normal = XMVector3TransformCoord(position, translationY);

		XMStoreFloat3(&v.normal, XMVector3Normalize(normal));

		v.texcoord = { float(i) / sliceCount, 0.0f };

		vertices.push_back(v);
	}

	vector<uint32_t>& indices = meshData.indices;

	for (int i = 0; i < sliceCount; i++) {
		indices.push_back(i);
		indices.push_back(i + sliceCount + 1);
		indices.push_back(i + sliceCount + 2);

		indices.push_back(i);
		indices.push_back(i + sliceCount + 2);
		indices.push_back(i + 1);
	}
	
	CalculateTangents(meshData);

	return meshData;
}

MeshData GeometryGenerator::MakeSphere(const float radius,
	const int numSlices, const int numStacks, const XMFLOAT2 texScale) {

	const float dTheta = -XM_2PI / float(numSlices);
	const float dPhi = -XM_PI / float(numStacks);

	MeshData meshData;

	vector<Vertex>& vertices = meshData.vertices;

	for (int j = 0; j <= numStacks; j++)
	{
		XMVECTOR startPosition = { 0.0f, -radius, 0.0f };
		XMMATRIX rotationZ = XMMatrixRotationZ(dPhi * float(j));

		startPosition = XMVector3TransformCoord(startPosition, rotationZ);

		for (int i = 0; i <= numSlices; i++) {
			Vertex v;

			XMMATRIX rotationY = XMMatrixRotationY(dTheta * float(i));
			XMVECTOR position = XMVector3TransformCoord(startPosition, rotationY);

			XMStoreFloat3(&v.position, position);

			XMStoreFloat3(&v.normal, XMVector3Normalize(position));

			v.texcoord = { float(i) / numSlices * texScale.x, 1.0f - float(j) / numStacks * texScale.y };

			XMVECTOR biTangent{ 0.0f, 1.0f, 0.0f };

			XMVECTOR normalOrth = position - XMVectorMultiply(XMVector3Dot(biTangent, position), position);
			normalOrth = XMVector3Normalize(normalOrth);

			XMStoreFloat3(&v.tangentModel, XMVector3Normalize(XMVector3Cross(biTangent, normalOrth)));

			vertices.push_back(v);
		}
	}

	vector<uint32_t>& indices = meshData.indices;

	for (int j = 0; j < numStacks; j++)
	{
		const int offset = (numSlices + 1) * j;

		for (int i = 0; i < numSlices; i++) {
			indices.push_back(offset + i);
			indices.push_back(offset + i + numSlices + 1);
			indices.push_back(offset + i + numSlices + 2);

			indices.push_back(offset + i);
			indices.push_back(offset + i + numSlices + 2);
			indices.push_back(offset + i + 1);
		}
	}

	return meshData;
}


void GeometryGenerator::CalculateTangents(MeshData& meshData)
{
	// 정점별 누적 탄젠트 초기화
	std::vector<XMVECTOR> accumulatedTangents(meshData.vertices.size(), XMVectorZero());

	// 삼각형 단위로 탄젠트 계산
	for (size_t i = 0; i < meshData.indices.size(); i += 3) {
		uint32_t i0 = meshData.indices[i];
		uint32_t i1 = meshData.indices[i + 1];
		uint32_t i2 = meshData.indices[i + 2];

		Vertex& v0 = meshData.vertices[i0];
		Vertex& v1 = meshData.vertices[i1];
		Vertex& v2 = meshData.vertices[i2];

		// DirectXMath를 사용한 벡터 연산
		XMVECTOR pos0 = XMLoadFloat3(&v0.position);
		XMVECTOR pos1 = XMLoadFloat3(&v1.position);
		XMVECTOR pos2 = XMLoadFloat3(&v2.position);

		XMVECTOR uv0 = XMLoadFloat2(&v0.texcoord);
		XMVECTOR uv1 = XMLoadFloat2(&v1.texcoord);
		XMVECTOR uv2 = XMLoadFloat2(&v2.texcoord);

		XMVECTOR edge1 = XMVectorSubtract(pos1, pos0);
		XMVECTOR edge2 = XMVectorSubtract(pos2, pos0);

		XMVECTOR deltaUV1 = XMVectorSubtract(uv1, uv0);
		XMVECTOR deltaUV2 = XMVectorSubtract(uv2, uv0);

		float deltaUV1x = XMVectorGetX(deltaUV1);
		float deltaUV1y = XMVectorGetY(deltaUV1);
		float deltaUV2x = XMVectorGetX(deltaUV2);
		float deltaUV2y = XMVectorGetY(deltaUV2);

		float r = 1.0f / (deltaUV1x * deltaUV2y - deltaUV1y * deltaUV2x);

		XMVECTOR tangent = XMVectorScale(
			XMVectorSubtract(
				XMVectorScale(edge1, deltaUV2y),
				XMVectorScale(edge2, deltaUV1y)
			),
			r
		);

		// 각 정점의 탄젠트에 누적
		accumulatedTangents[i0] = XMVectorAdd(accumulatedTangents[i0], tangent);
		accumulatedTangents[i1] = XMVectorAdd(accumulatedTangents[i1], tangent);
		accumulatedTangents[i2] = XMVectorAdd(accumulatedTangents[i2], tangent);
	}

	// 정점마다 누적된 탄젠트를 정규화하여 저장
	for (size_t i = 0; i < meshData.vertices.size(); ++i) {
		XMVECTOR tangent = XMVector3Normalize(accumulatedTangents[i]);
		XMStoreFloat3(&meshData.vertices[i].tangentModel, tangent);
	}
}