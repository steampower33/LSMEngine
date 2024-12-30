#pragma once

#include "MeshData.h"
#include "ModelLoader.h"

#include <string>

using namespace std;

class GeometryGenerator {
public:
	static MeshData MakeBox();
	static vector<MeshData> ReadFromFile(string basePath, string filename);
};