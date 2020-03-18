#include "Material.hpp"

#include <commdlg.h>
#include <tchar.h>

#include <assimp/material.h>

FMaterial::FMaterial(FRenderer& Renderer): InternalRenderer(Renderer)
{
}

FMaterial::~FMaterial()
{
	InternalRenderer.DestroyTexture(Albedo);
	InternalRenderer.DestroyTexture(Metalness);
	InternalRenderer.DestroyTexture(Roughness);
	InternalRenderer.DestroyTexture(Normal);

	InternalRenderer.DestroyShader(Shader);
	InternalRenderer.DestroyBuffer(ConstantBuffer);
}

void FMaterial::Initialize(const uint32_t Width, const uint32_t Height) noexcept
{
	if (bIsInitialized)
	{
		return;
	}
	D3D11_INPUT_ELEMENT_DESC InputElementDescriptors[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	uint8_t ShockingPink[4] = {252, 15, 192, 255};
	InternalRenderer.CreateTextureFromMemory(ShockingPink, 1, 1, 4, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, Albedo);
	InternalRenderer.CreateTextureFromMemory(ShockingPink, 1, 1, 4, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, Metalness);
	InternalRenderer.CreateTextureFromMemory(ShockingPink, 1, 1, 4, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, Roughness);
	InternalRenderer.CreateTextureFromMemory(ShockingPink, 1, 1, 4, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, Normal);
	InternalRenderer.CreateVertexShader(L"DefaultVS.hlsl", "main", InputElementDescriptors, 5, Shader);
	InternalRenderer.CreatePixelShader(L"DefaultPS.hlsl", "main", Shader);

	InternalRenderer.CreateConstantBufferWithData(MaterialConstantBuffer, ConstantBuffer);

	bIsInitialized = true;
}

void FMaterial::LoadMaterial(const std::string& RootDir, const aiMaterial* Material) noexcept
{
	aiString FileName{};
	Material->GetTexture(aiTextureType_DIFFUSE, 0, &FileName);
	auto Path = RootDir + std::string(FileName.C_Str());
	ReloadTexture(Path.c_str(), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, Albedo);

	FileName.Clear();
	Path.clear();
	Material->GetTexture(aiTextureType_AMBIENT, 0, &FileName);
	Path = RootDir + std::string(FileName.C_Str());
	ReloadTexture(Path.c_str(), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, Metalness);

	FileName.Clear();
	Path.clear();
	Material->GetTexture(aiTextureType_SHININESS, 0, &FileName);
	Path = RootDir + std::string(FileName.C_Str());
	ReloadTexture(Path.c_str(), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, Roughness);

	FileName.Clear();
	Path.clear();
	Material->GetTexture(aiTextureType_HEIGHT, 0, &FileName);
	Path = RootDir + std::string(FileName.C_Str());
	ReloadTexture(Path.c_str(), DXGI_FORMAT_R8G8B8A8_UNORM, Normal);
}

void FMaterial::ReloadTexture(const DXGI_FORMAT Format, SRenderTarget& RenderTarget) const noexcept
{
	TCHAR File[MAX_PATH] = {0};

	OPENFILENAME OpenFileName{};
	OpenFileName.lStructSize = sizeof(OpenFileName);
	OpenFileName.hwndOwner = nullptr;
	OpenFileName.lpstrFile = File;
	OpenFileName.nMaxFile = sizeof(File);
	OpenFileName.lpstrFilter = _T("(*.*) All\0*.*\0(*.png) PNG\0*.png\0(*.jpg) JPG\0*.jpg\0(*.dds) DDS\0*.dds\0");
	OpenFileName.nFilterIndex = 1;
	OpenFileName.lpstrFileTitle = nullptr;
	OpenFileName.nMaxFileTitle = 0;
	OpenFileName.lpstrInitialDir = nullptr;
	OpenFileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&OpenFileName) == TRUE)
	{
		ReloadTexture(OpenFileName.lpstrFile, Format, RenderTarget);
	}
}

void FMaterial::ReloadTexture(const char* FileName, const DXGI_FORMAT Format, SRenderTarget& RenderTarget) const noexcept
{
	SRenderTarget TemporaryRenderTarget;

	const auto Result = InternalRenderer.CreateTextureFromFile(FileName, Format, TemporaryRenderTarget);
	if (Result == EErrorCode::OK)
	{
		InternalRenderer.DestroyTexture(RenderTarget);
		RenderTarget = TemporaryRenderTarget;
	}
}

void FMaterial::OnGui() noexcept
{
	ImGui::BeginChild("Material Editor");
	{
		const uint32_t TexturePreviewSize = 160;

		auto CurrentItem = static_cast<int>(MaterialConstantBuffer.Illumination);
		ImGui::Combo("Illumination Model", &CurrentItem, "Disney\0Cook-Torrance\0\0");
		
		MaterialConstantBuffer.Illumination = static_cast<EIlluminationModel>(CurrentItem);
		
		{
			ImGui::Text("Albedo");
			ImGui::SameLine(ImGui::GetWindowWidth() - TexturePreviewSize - 30);
			ImGui::PushID(0);
			if (ImGui::ImageButton(Albedo.ShaderResourceView, ImVec2(TexturePreviewSize, TexturePreviewSize)))
			{
				ReloadTexture(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, Albedo);
			}
			ImGui::PopID();
		}
		{
			ImGui::Text("Metalness");
			ImGui::SameLine(ImGui::GetWindowWidth() - TexturePreviewSize - 30);
			ImGui::PushID(1);
			if (ImGui::ImageButton(Metalness.ShaderResourceView, ImVec2(TexturePreviewSize, TexturePreviewSize)))
			{
				ReloadTexture(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, Metalness);
			}
			ImGui::PopID();
		}
		{
			ImGui::Text("Roughness");
			ImGui::SameLine(ImGui::GetWindowWidth() - TexturePreviewSize - 30);
			ImGui::PushID(2);
			if (ImGui::ImageButton(Roughness.ShaderResourceView, ImVec2(TexturePreviewSize, TexturePreviewSize)))
			{
				ReloadTexture(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, Roughness);
			}
			ImGui::PopID();
		}
		{
			ImGui::Text("Normal");
			ImGui::SameLine(ImGui::GetWindowWidth() - TexturePreviewSize - 30);
			ImGui::PushID(3);
			if (ImGui::ImageButton(Normal.ShaderResourceView, ImVec2(TexturePreviewSize, TexturePreviewSize)))
			{
				ReloadTexture(DXGI_FORMAT_R8G8B8A8_UNORM, Normal);
			}
			ImGui::PopID();
		}
	}
	ImGui::EndChild();
}

void FMaterial::OnRender(const SRenderTarget* RenderTargets, const size_t Count) noexcept
{
	InternalRenderer.SetShader(Shader);
	InternalRenderer.SetConstantBuffer(ConstantBuffer, EShaderStage::PIXEL, 2);
	InternalRenderer.UpdateSubresource(ConstantBuffer, &MaterialConstantBuffer, sizeof(struct SMaterialConstantBuffer));

	InternalRenderer.SetTexture(0, Albedo);
	InternalRenderer.SetTexture(1, Metalness);
	InternalRenderer.SetTexture(2, Roughness);
	InternalRenderer.SetTexture(3, Normal);
}
