#pragma once

#include "Renderer.hpp"

class FBlurMaterial
{
private:
	struct SBlurParams
	{
		float Smooth = 0.963f;			// 0.0 -> box filter, > 0.0 for gaussian
		float Size = 0.643f;				// length of the blur (global)
		float SamplesX = 0.352f;			// number of samples to take
		float SamplesY = 0.536f;			// number of samples to take
		float DirectionX = 0.488f;		// direction of blur
		float DirectionY = 0.664f;		// direction of blur
		float PowerX = 0.376f;			// length of the blur
		float PowerY = 0.423f;			// length of the blur
	};

public:
	explicit FBlurMaterial(FRenderer& Renderer);
	~FBlurMaterial();

	EErrorCode Initialize(const uint32_t Width, const uint32_t Height) noexcept;
	void OnGui() noexcept;
	void OnRender(const SRenderTarget* RenderTargets, const size_t Count) noexcept;

	void SetMask(const SRenderTarget& Mask) noexcept;
	const SRenderTarget& GetResult() const noexcept;
	bool IsEnabled() const noexcept;

private:
	FRenderer& InternalRenderer;
	SBlurParams BlurParams{};
	
	SShader BlurXShader{};
	SShader BlurYShader{};

	SBuffer BlurConstantBuffer{};

	SRenderTarget RenderTarget{};
	SRenderTarget FinalRenderTarget{};

	SRenderTarget MaskTexture{};

	bool bIsEnabled = true;
};