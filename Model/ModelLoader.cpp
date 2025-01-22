#include "ModelLoader.h"

void ModelLoader::Load(std::string basePath, std::string filename)
{
	this->basePath = basePath;

	Assimp::Importer importer;

	if (GetExtension(filename) == ".gltf") {
		m_isGLTF = true;
	}

	const aiScene* pScene = importer.ReadFile(
		this->basePath + filename,
		aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);

	if (!pScene || !pScene->mRootNode)
	{
		assert(false && "Model Node does not exist!");
	}

	// 초기 변환 행렬(단위 행렬)
	XMMATRIX identityMatrix = XMMatrixIdentity();

	// 루트 노드부터 순회 시작
	ProcessNode(pScene->mRootNode, pScene, identityMatrix);
}

void ModelLoader::ProcessNode(aiNode* node, const aiScene* scene, XMMATRIX parentTransform)
{
	std::cout << node->mName.C_Str() << " : " << node->mNumMeshes << " "
		<< node->mNumChildren << std::endl;

	aiMatrix4x4& aiMat = node->mTransformation;
	XMMATRIX nodeMatrix = XMMATRIX(
		aiMat.a1, aiMat.a2, aiMat.a3, aiMat.a4,
		aiMat.b1, aiMat.b2, aiMat.b3, aiMat.b4,
		aiMat.c1, aiMat.c2, aiMat.c3, aiMat.c4,
		aiMat.d1, aiMat.d2, aiMat.d3, aiMat.d4
	);

	nodeMatrix = XMMatrixTranspose(nodeMatrix);

	XMMATRIX worldMatrix = XMMatrixMultiply(nodeMatrix, parentTransform);

	for (uint32_t i = 0; i < node->mNumChildren; i++) {
		ProcessNode(node->mChildren[i], scene, parentTransform);
	}

	// 3) 이 노드에 연결된 모든 메시를 처리
	for (uint32_t i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		MeshData newMesh = this->ProcessMesh(mesh, scene);

		// 메시의 모든 정점에 "worldMatrix"를 적용
		for (auto& v : newMesh.vertices)
		{
			XMVECTOR posVec = XMLoadFloat3(&v.position);
			// 위치 변환
			posVec = XMVector3Transform(posVec, worldMatrix);
			XMStoreFloat3(&v.position, posVec);
		}

		// (만약 노멀도 부모/노드 변환 적용이 필요하면 XMVector3TransformNormal)
		// 단, 비균일 스케일 등 있으면 InverseTranspose 등을 써야 함
		// 여기선 노멀은 Assimp가 이미 로컬 공간에서 구해주므로,
		// 필요하다면 "XMMatrixInverseTranspose(worldMatrix)" 계산 후 곱하기.

		meshes.push_back(newMesh);

		// 4) 자식 노드 처리 (재귀)
		for (uint32_t i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(node->mChildren[i], scene, worldMatrix);
		}
	}
}

string ModelLoader::GetExtension(const string filename) {
	string ext(filesystem::path(filename).extension().string());
	transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	return ext;
}

MeshData ModelLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	vertices.reserve(mesh->mNumVertices);
	indices.reserve(mesh->mNumFaces * 3); // Triangulate로 인해 Face는 보통 3 인덱스

	// 1) 정점 정보
	for (uint32_t i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex = {};

		// Position
		vertex.position.x = mesh->mVertices[i].x;
		vertex.position.y = mesh->mVertices[i].y;
		vertex.position.z = mesh->mVertices[i].z;

		// Normal
		if (mesh->mNormals)
		{
			XMFLOAT3 normal;

			normal.x = mesh->mNormals[i].x;

			if (m_isGLTF) {
				normal.y = mesh->mNormals[i].z;
				normal.z = -mesh->mNormals[i].y;
			}
			else {
				normal.y = mesh->mNormals[i].y;
				normal.z = mesh->mNormals[i].z;
			}

			// 정규화
			XMVECTOR n = XMLoadFloat3(&normal);
			n = XMVector3Normalize(n);
			XMStoreFloat3(&vertex.normal, n);
		}

		// UV (채널 0만 사용)
		if (mesh->mTextureCoords[0])
		{
			vertex.texcoord.x = (float)mesh->mTextureCoords[0][i].x;
			vertex.texcoord.y = (float)mesh->mTextureCoords[0][i].y;
		}

		vertices.push_back(vertex);
	}

	// 2) 인덱스 정보
	for (uint32_t i = 0; i < mesh->mNumFaces; i++)
	{
		const aiFace& face = mesh->mFaces[i];
		for (uint32_t j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	// 3) 머티리얼 / 텍스처
	MeshData newMesh;
	newMesh.vertices = vertices;
	newMesh.indices = indices;

	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		newMesh.albedoFilename =
			ReadFilename(material, aiTextureType_BASE_COLOR);
		if (newMesh.albedoFilename.empty()) {
			newMesh.albedoFilename =
				ReadFilename(material, aiTextureType_DIFFUSE);
		}

		newMesh.normalFilename =
			ReadFilename(material, aiTextureType_NORMALS);
		newMesh.heightFilename =
			ReadFilename(material, aiTextureType_HEIGHT);
		newMesh.aoFilename =
			ReadFilename(material, aiTextureType_AMBIENT_OCCLUSION);
		if (newMesh.aoFilename.empty()) {
			newMesh.aoFilename =
				ReadFilename(material, aiTextureType_LIGHTMAP);
		}
		newMesh.metallicFilename =
			ReadFilename(material, aiTextureType_METALNESS);
		newMesh.roughnessFilename =
			ReadFilename(material, aiTextureType_DIFFUSE_ROUGHNESS);
		newMesh.emissiveFilename =
			ReadFilename(material, aiTextureType_EMISSIVE);
	}

	return newMesh;
}


std::string ModelLoader::ReadFilename(aiMaterial* material, aiTextureType type) {

	if (material->GetTextureCount(type) > 0) {
		aiString filepath;
		material->GetTexture(type, 0, &filepath);

		std::string fullPath =
			this->basePath +
			std::string(
				std::filesystem::path(filepath.C_Str()).filename().string());

		return fullPath;
	}
	else {
		return "";
	}
}