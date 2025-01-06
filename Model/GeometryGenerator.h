#pragma once

#include "MeshData.h"
#include "ModelLoader.h"

#include <string>

using namespace std;
using namespace DirectX;

class GeometryGenerator {
public:
	static vector<MeshData> ReadFromFile(string basePath, string filename);

	static MeshData MakeBox(const float scale);
	static MeshData MakeCylinder(
		const float bottomRadius,
		const float topRadius, float height,
		int sliceCount);
};