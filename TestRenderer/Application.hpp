#pragma once

#define NOMINMAX
#include <windows.h>
#include "Renderer.hpp"
#include "Camera.hpp"
#include "BlurMaterial.hpp"
#include "Model.hpp"
#include "TexGen.hpp"
#include "Light.hpp"

class FApplication
{
public:
	EErrorCode Setup(const HWND HWnd, const uint32_t Width, const uint32_t Height);

	void OnRender() noexcept;
	void OnUpdate(const float Time) noexcept;
	void OnGui() noexcept;
	
	void TearDown() noexcept;
	void Resize(const uint32_t Width, const uint32_t Height) const noexcept;
	EErrorCode Present() const noexcept;

	static double GetHighResolutionTime() noexcept;

private:
	SRenderTarget BackBuffer{};
	SRenderTarget SceneRenderTarget{};
	FRenderer Renderer{};
	Generator::FTexGen TexGen{ Renderer };
	FCamera MainCamera{ Renderer };
	
	FBlurMaterial Blur{ Renderer };
	
	FModel Model{ Renderer, MainCamera};
	FLight Light{ Renderer };
};
