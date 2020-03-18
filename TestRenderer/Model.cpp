#include "Model.hpp"

#define NOMINMAX
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <tchar.h>

FModel::FModel(FRenderer& Renderer, FCamera& Camera) : InternalRenderer(Renderer), InternalCamera(Camera)
{
}

FModel::~FModel()
{
	for (auto& Mesh : Meshes)
	{
		InternalRenderer.DestroyBuffer(Mesh.VertexBuffer);
		InternalRenderer.DestroyBuffer(Mesh.IndexBuffer);
	}
	InternalRenderer.DestroyBuffer(TransformConstantBuffer);
}

EErrorCode FModel::Initialize(const char* Path, const uint32_t Width, const uint32_t Height)
{
	FilePath = Path;
	Material.Initialize(Width, Height);
	Assimp::Importer Importer;
	const aiScene* Scene = Importer.ReadFile(Path,
		aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded |
		aiProcess_CalcTangentSpace |
		aiProcess_GenUVCoords |
		aiProcess_ForceGenNormals |
		aiProcess_ImproveCacheLocality |
		aiProcess_FindInvalidData |
		aiProcess_OptimizeMeshes |
		aiProcess_OptimizeGraph |
		aiProcess_JoinIdenticalVertices |
		aiProcess_FindDegenerates);
	
	if (!Scene || Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !Scene->mRootNode)
	{
		return EErrorCode::FAIL;
	}
	
	ProcessNode(Scene->mRootNode, Scene);

	PerFrame.World = DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity());
	PerFrame.View = DirectX::XMMatrixTranspose(InternalCamera.GetViewMatrix());
	PerFrame.Projection = DirectX::XMMatrixTranspose(InternalCamera.GetProjectionMatrix());

	InternalRenderer.CreateConstantBufferWithData(PerFrame, TransformConstantBuffer);
	return EErrorCode::OK;
}

void FModel::ProcessNode(aiNode* Node, const aiScene* Scene)
{
	for (size_t i = 0; i < Node->mNumMeshes; ++i)
	{
		aiMesh* Mesh = Scene->mMeshes[Node->mMeshes[i]];
		Meshes.push_back(ProcessMesh(Mesh, Scene));
	}
	for (unsigned int i = 0; i < Node->mNumChildren; i++)
	{
		ProcessNode(Node->mChildren[i], Scene);
	}
}

FModel::SMesh FModel::ProcessMesh(aiMesh* Mesh, const aiScene* Scene)
{
	const auto Vertices = new SVertex[Mesh->mNumVertices];
	std::vector<uint32_t> Indices;

	for (size_t i = 0; i < Mesh->mNumVertices; ++i)
	{
		SVertex Vertex{};
		Vertex.Position.x = Mesh->mVertices[i].x;
		Vertex.Position.y = Mesh->mVertices[i].y;
		Vertex.Position.z = Mesh->mVertices[i].z;

		Vertex.Normal.x = Mesh->mNormals[i].x;
		Vertex.Normal.y = Mesh->mNormals[i].y;
		Vertex.Normal.z = Mesh->mNormals[i].z;

		if (Mesh->mTextureCoords[0])
		{
			Vertex.TexCoord.x = Mesh->mTextureCoords[0][i].x;
			Vertex.TexCoord.y = Mesh->mTextureCoords[0][i].y;
		}
		else
		{
			Vertex.TexCoord = { 0.0f, 0.0f };
		}
		
		if (Mesh->mTangents)
		{
			Vertex.Tangent.x = Mesh->mTangents[i].x;
			Vertex.Tangent.y = Mesh->mTangents[i].y;
			Vertex.Tangent.z = Mesh->mTangents[i].z;
		}
		else
		{
			Vertex.Tangent = { 0, 0, 0 };
		}
		
		if (Mesh->mBitangents)
		{
			Vertex.Bitangent.x = Mesh->mBitangents[i].x;
			Vertex.Bitangent.y = Mesh->mBitangents[i].y;
			Vertex.Bitangent.z = Mesh->mBitangents[i].z;
		}
		else
		{
			Vertex.Bitangent = { 0, 0, 0 };
		}
		Vertices[i] = Vertex;
	}
	for (size_t i = 0; i < Mesh->mNumFaces; ++i)
	{
		const aiFace Face = Mesh->mFaces[i];
		for (size_t j = 0; j < Face.mNumIndices; ++j)
		{
			Indices.push_back(Face.mIndices[j]);
		}
	}
	Material.LoadMaterial(FilePath.substr(0, FilePath.find_last_of("/\\") + 1),
	                      Scene->mMaterials[Mesh->mMaterialIndex]);

	SMesh TempMesh{};
	InternalRenderer.CreateVertexBufferWithData(Vertices, Mesh->mNumVertices, TempMesh.VertexBuffer);
	InternalRenderer.CreateIndexBufferWithData(Indices.data(), Indices.size(), TempMesh.IndexBuffer);
	TempMesh.IndexCount = Indices.size();
	delete[] Vertices;

	return TempMesh;
}

void FModel::OnUpdate(const float Time) noexcept
{
	PerFrame.World = DirectX::XMMatrixTranspose(DirectX::XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z));
	PerFrame.View = DirectX::XMMatrixTranspose(InternalCamera.GetViewMatrix());
	PerFrame.Projection = DirectX::XMMatrixTranspose(InternalCamera.GetProjectionMatrix());
}

void FModel::OnGui() noexcept
{
	ImGui::Begin("Model");
	{
		if (ImGui::Button("Load Mesh"))
		{
			TCHAR File[MAX_PATH] = { 0 };

			OPENFILENAME OpenFileName{};
			OpenFileName.lStructSize = sizeof(OpenFileName);
			OpenFileName.hwndOwner = nullptr;
			OpenFileName.lpstrFile = File;
			OpenFileName.nMaxFile = sizeof(File);
			OpenFileName.lpstrFilter = _T("(*.*) All\0*.*\0(*.obj) Wavefront OBJ\0*.obj\0");
			OpenFileName.nFilterIndex = 1;
			OpenFileName.lpstrFileTitle = nullptr;
			OpenFileName.nMaxFileTitle = 0;
			OpenFileName.lpstrInitialDir = nullptr;
			OpenFileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			if (GetOpenFileName(&OpenFileName) == TRUE)
			{
				for (auto& Mesh : Meshes)
				{
					InternalRenderer.DestroyBuffer(Mesh.VertexBuffer);
					InternalRenderer.DestroyBuffer(Mesh.IndexBuffer);
				}
				Meshes.clear();
				Initialize(OpenFileName.lpstrFile, 0, 0);
			}
		}
		ImGui::DragFloat3("Rotation", &Rotation.x, 0.001f);
		Material.OnGui();
		InternalCamera.OnGui();
	}
	ImGui::End();
}

void FModel::OnRender(const SRenderTarget* RenderTargets, const size_t Count)  noexcept
{
	Material.OnRender();
	InternalCamera.OnRender();

	InternalRenderer.SetConstantBuffer(TransformConstantBuffer, EShaderStage::VERTEX);
	InternalRenderer.UpdateSubresource(TransformConstantBuffer, &PerFrame, sizeof(SPerFrame));
	for (auto& Mesh : Meshes)
	{
		InternalRenderer.SetVertexBuffer(0, Mesh.VertexBuffer, 0);
		InternalRenderer.SetIndexBuffer(0, Mesh.IndexBuffer, 0);
		InternalRenderer.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		InternalRenderer.DrawIndexed(Mesh.IndexCount);
	}
}