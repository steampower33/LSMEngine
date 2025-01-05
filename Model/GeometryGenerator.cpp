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
	using namespace DirectX;

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