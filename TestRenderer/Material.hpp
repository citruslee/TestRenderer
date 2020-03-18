#pragma once

#include "Renderer.hpp"
#include <string>

struct aiMaterial;

class FMaterial
{
public:
	explicit FMaterial(FRenderer& Renderer);

	~FMaterial();
	
	void LoadMaterial(const std::string& RootDir, const aiMaterial* Material) noexcept;
	void ReloadTexture(const DXGI_FORMAT Format, SRenderTarget& RenderTarget) const noexcept;
	void ReloadTexture(const char* FileName, const DXGI_FORMAT Format, SRenderTarget& RenderTarget) const noexcept;

	void Initialize(const uint32_t Width, const uint32_t Height) noexcept;
	void OnGui() noexcept;
	void OnRender(const SRenderTarget* RenderTargets = nullptr, const size_t Count = 0) noexcept;

private:
	FRenderer& InternalRenderer;

	SRenderTarget Albedo{};
	SRenderTarget Metalness{};
	SRenderTarget Roughness{};
	SRenderTarget Normal{};
	SShader Shader{};
	SBuffer ConstantBuffer{};

	enum class EIlluminationModel : uint32_t
	{
		DISNEY = 0,
		COOKTORRANCE,
		COUNT
	};

	struct SMaterialConstantBuffer
	{
		EIlluminationModel Illumination = EIlluminationModel::DISNEY;
	};

	SMaterialConstantBuffer MaterialConstantBuffer{};

	bool bIsInitialized = false;
};
