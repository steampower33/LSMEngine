#include "ModelLoader.h"

#include <filesystem>

void ModelLoader::Load(std::string basePath, std::string filename)
{
	this->basePath = basePath;

	Assimp::Importer importer;

	const aiScene* pScene = importer.ReadFile(
		this->basePath + filename,
		aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);

	if (!pScene || !pScene->mRootNode)
	{
		std::cerr << "Assimp failed: " << importer.GetErrorString() << std::endl;
		return;
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

	for (int i = 0; i < node->mNumChildren; i++) {
		ProcessNode(node->mChildren[i], scene, parentTransform);
	}

	// 3) 이 노드에 연결된 모든 메시를 처리
	// 나중에 잘봐라 이거 UINT 헤더 문제 생기는지
	for (int i = 0; i < node->mNumMeshes; i++)
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
		for (int i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(node->mChildren[i], scene, worldMatrix);
		}
	}
}


MeshData ModelLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	vertices.reserve(mesh->mNumVertices);
	indices.reserve(mesh->mNumFaces * 3); // Triangulate로 인해 Face는 보통 3 인덱스

	// 1) 정점 정보
	for (int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex = {};

		// Position
		vertex.position.x = mesh->mVertices[i].x;
		vertex.position.y = mesh->mVertices[i].y;
		vertex.position.z = mesh->mVertices[i].z;

		// Normal
		if (mesh->mNormals)
		{
			XMFLOAT3 normal(
				mesh->mNormals[i].x,
				mesh->mNormals[i].y,
				mesh->mNormals[i].z
			);
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
	for (int i = 0; i < mesh->mNumFaces; i++)
	{
		const aiFace& face = mesh->mFaces[i];
		for (int j = 0; j < face.mNumIndices; j++)
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

		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
		{
			aiString filepath;
			if (AI_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, 0, &filepath))
			{
				// basePath와 합쳐서 텍스처 전체 경로를 구성
				std::string fullPath =
					this->basePath +
					std::string(std::filesystem::path(filepath.C_Str()).filename().string());

				newMesh.textureFilename = fullPath;
			}
		}
	}

	return newMesh;
}
