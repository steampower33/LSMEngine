#include "GeometryGenerator.h"

MeshData GeometryGenerator::MakeBox(const float scale) {
	MeshData meshData;

	meshData.vertices =
	{
		// front
		{ -1.0f * scale, -1.0f * scale, -1.0f * scale, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f },
		{ -1.0f * scale,  1.0f * scale, -1.0f * scale, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f },
		{  1.0f * scale,  1.0f * scale, -1.0f * scale, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f },
		{  1.0f * scale, -1.0f * scale, -1.0f * scale, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f },

		// back
		{  1.0f * scale, -1.0f * scale,  1.0f * scale, 0.0f, 1.0f, 0.0f, 0.0f,  1.0f },
		{  1.0f * scale,  1.0f * scale,  1.0f * scale, 0.0f, 0.0f, 0.0f, 0.0f,  1.0f },
		{ -1.0f * scale,  1.0f * scale,  1.0f * scale, 1.0f, 0.0f, 0.0f, 0.0f,  1.0f },
		{ -1.0f * scale, -1.0f * scale,  1.0f * scale, 1.0f, 1.0f, 0.0f, 0.0f,  1.0f },

		// top
		{ -1.0f * scale,  1.0f * scale, -1.0f * scale, 0.0f, 1.0f, 0.0f,  1.0f, 0.0f },
		{ -1.0f * scale,  1.0f * scale,  1.0f * scale, 0.0f, 0.0f, 0.0f,  1.0f, 0.0f },
		{  1.0f * scale,  1.0f * scale,  1.0f * scale, 1.0f, 0.0f, 0.0f,  1.0f, 0.0f },
		{  1.0f * scale,  1.0f * scale, -1.0f * scale, 1.0f, 1.0f, 0.0f,  1.0f, 0.0f },

		// bottom
		{ -1.0f * scale, -1.0f * scale,  1.0f * scale, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f },
		{ -1.0f * scale, -1.0f * scale, -1.0f * scale, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f },
		{  1.0f * scale, -1.0f * scale, -1.0f * scale, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f },
		{  1.0f * scale, -1.0f * scale,  1.0f * scale, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f },

		// left
		{ -1.0f * scale, -1.0f * scale,  1.0f * scale, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f },
		{ -1.0f * scale,  1.0f * scale,  1.0f * scale, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f },
		{ -1.0f * scale,  1.0f * scale, -1.0f * scale, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f },
		{ -1.0f * scale, -1.0f * scale, -1.0f * scale, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f },

		// right
		{  1.0f * scale, -1.0f * scale, -1.0f * scale, 0.0f, 1.0f,  1.0f, 0.0f, 0.0f },
		{  1.0f * scale,  1.0f * scale, -1.0f * scale, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f },
		{  1.0f * scale,  1.0f * scale,  1.0f * scale, 1.0f, 0.0f,  1.0f, 0.0f, 0.0f },
		{  1.0f * scale, -1.0f * scale,  1.0f * scale, 1.0f, 1.0f,  1.0f, 0.0f, 0.0f },
	};

	meshData.indices = {
			0, 1, 2, 0, 2, 3,
			4, 5, 6, 4, 6, 7,
			8, 9, 10, 8, 10, 11,
			12, 13, 14, 12, 14, 15,
			16, 17, 18, 16, 18, 19,
			20, 21, 22, 20, 22, 23,
	};

	return meshData;
}


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

	return meshes;
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

	return meshData;
}

MeshData GeometryGenerator::MakeSphere(const float radius,
	const int numSlices, const int numStacks) {

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

			v.texcoord = { float(i) / numSlices, 1.0f - float(j) / numStacks};

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