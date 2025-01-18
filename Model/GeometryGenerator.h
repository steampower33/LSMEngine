#pragma once

#include "MeshData.h"
#include "ModelLoader.h"

#include <string>

using namespace std;
using namespace DirectX;

class GeometryGenerator {
public:
	static vector<MeshData> ReadFromFile(string basePath, string filename);

	static MeshData MakeSquare(const float scale);
	static MeshData MakeBox(const float scale);
	static MeshData MakeCylinder(
		const float bottomRadius,
		const float topRadius, float height,
		int sliceCount);
	static MeshData MakeSphere(
		const float radius, 
		const int numSlices, const int numStacks);
	static void CalculateTangents(MeshData& meshData);

};