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

	// �ʱ� ��ȯ ���(���� ���)
	XMMATRIX identityMatrix = XMMatrixIdentity();

	// ��Ʈ ������ ��ȸ ����
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

	// 3) �� ��忡 ����� ��� �޽ø� ó��
	// ���߿� �ߺ��� �̰� UINT ��� ���� �������
	for (int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		MeshData newMesh = this->ProcessMesh(mesh, scene);

		// �޽��� ��� ������ "worldMatrix"�� ����
		for (auto& v : newMesh.vertices)
		{
			XMVECTOR posVec = XMLoadFloat3(&v.position);
			// ��ġ ��ȯ
			posVec = XMVector3Transform(posVec, worldMatrix);
			XMStoreFloat3(&v.position, posVec);
		}

		// (���� ��ֵ� �θ�/��� ��ȯ ������ �ʿ��ϸ� XMVector3TransformNormal)
		// ��, ����� ������ �� ������ InverseTranspose ���� ��� ��
		// ���⼱ ����� Assimp�� �̹� ���� �������� �����ֹǷ�,
		// �ʿ��ϴٸ� "XMMatrixInverseTranspose(worldMatrix)" ��� �� ���ϱ�.

		meshes.push_back(newMesh);

		// 4) �ڽ� ��� ó�� (���)
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
	indices.reserve(mesh->mNumFaces * 3); // Triangulate�� ���� Face�� ���� 3 �ε���

	// 1) ���� ����
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
			// ����ȭ
			XMVECTOR n = XMLoadFloat3(&normal);
			n = XMVector3Normalize(n);
			XMStoreFloat3(&vertex.normal, n);
		}

		// UV (ä�� 0�� ���)
		if (mesh->mTextureCoords[0])
		{
			vertex.texcoord.x = (float)mesh->mTextureCoords[0][i].x;
			vertex.texcoord.y = (float)mesh->mTextureCoords[0][i].y;
		}

		vertices.push_back(vertex);
	}

	// 2) �ε��� ����
	for (int i = 0; i < mesh->mNumFaces; i++)
	{
		const aiFace& face = mesh->mFaces[i];
		for (int j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	// 3) ��Ƽ���� / �ؽ�ó
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
				// basePath�� ���ļ� �ؽ�ó ��ü ��θ� ����
				std::string fullPath =
					this->basePath +
					std::string(std::filesystem::path(filepath.C_Str()).filename().string());

				newMesh.textureFilename = fullPath;
			}
		}
	}

	return newMesh;
}
