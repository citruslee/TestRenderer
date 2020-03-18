#pragma once
#include "Renderer.hpp"
#include "Camera.hpp"
#include "Material.hpp"
#include <assimp/scene.h>
#include <DirectXMath.h>
#include <vector>

class FModel
{
private:
	struct SVertex
	{
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT2 TexCoord;
		DirectX::XMFLOAT3 Tangent;
		DirectX::XMFLOAT3 Bitangent;
	};

	struct SMesh
	{
		SBuffer VertexBuffer;
		SBuffer IndexBuffer;
		size_t IndexCount;
	};

	struct SPerFrame
	{
		DirectX::XMMATRIX World;
		DirectX::XMMATRIX View;
		DirectX::XMMATRIX Projection;
	};

public:

	explicit FModel(FRenderer& Renderer, FCamera& Camera);
	~FModel();
	
	EErrorCode Initialize(const char* Path, const uint32_t Width, const uint32_t Height);
	void OnUpdate(const float Time) noexcept;
	void OnGui() noexcept;
	void OnRender(const SRenderTarget* RenderTargets, const size_t Count) noexcept;

	void ProcessNode(aiNode* Node, const aiScene* Scene);
	SMesh ProcessMesh(aiMesh* Mesh, const aiScene* Scene);
private:
	FRenderer& InternalRenderer;
	FCamera& InternalCamera;
	FMaterial Material{ InternalRenderer };

	DirectX::XMFLOAT3 Rotation;

	std::string FilePath;

	std::vector<SMesh> Meshes;
	
	SBuffer TransformConstantBuffer{};
	SPerFrame PerFrame{};
};

