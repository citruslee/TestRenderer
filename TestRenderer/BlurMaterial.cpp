#include "BlurMaterial.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

FBlurMaterial::FBlurMaterial(FRenderer& Renderer) : InternalRenderer(Renderer)
{
	bIsEnabled = false;
}

FBlurMaterial::~FBlurMaterial()
{
	InternalRenderer.DestroyShader(BlurXShader);
	InternalRenderer.DestroyShader(BlurYShader);;

	InternalRenderer.DestroyBuffer(BlurConstantBuffer);

	InternalRenderer.DestroyRenderTarget(RenderTarget);
	InternalRenderer.DestroyRenderTarget(FinalRenderTarget);

	InternalRenderer.DestroyTexture(MaskTexture);
}

EErrorCode FBlurMaterial::Initialize(const uint32_t Width, const uint32_t Height) noexcept
{
#define CHECK_RESULT() \
	if (Result != EErrorCode::OK)\
	{\
		return Result;\
	}
	auto Result = InternalRenderer.CreateVertexShader(L"FullScreenTriangleVS.hlsl", "main", nullptr, 0, BlurXShader);
	CHECK_RESULT();
	Result = InternalRenderer.CreatePixelShader(L"BlurXPS.hlsl", "main", BlurXShader);
	CHECK_RESULT();
	Result = InternalRenderer.CreateVertexShader(L"FullScreenTriangleVS.hlsl", "main", nullptr, 0, BlurYShader);
	CHECK_RESULT();
	Result = InternalRenderer.CreatePixelShader(L"BlurYPS.hlsl", "main", BlurYShader);
	CHECK_RESULT();
	Result = InternalRenderer.CreateConstantBufferWithData(BlurParams, BlurConstantBuffer);
	CHECK_RESULT();
	auto* MaskBuffer = new uint8_t[Width * Height];
	for (size_t Row = 0; Row < Height; ++Row)
	{
		memset(MaskBuffer + (Row * Width), 0, Width);
		memset(MaskBuffer + (Row * Width + static_cast<uint32_t>(Width * 0.5f)), 255, static_cast<uint32_t>(Width * 0.5f));
	}
	Result = InternalRenderer.CreateTextureFromMemory(MaskBuffer, Width, Height, 1, DXGI_FORMAT_R8_UNORM, MaskTexture);
	delete[] MaskBuffer;
	CHECK_RESULT();

	Result = InternalRenderer.CreateRenderTarget(Width, Height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, RenderTarget);
	CHECK_RESULT();
	Result = InternalRenderer.CreateRenderTarget(Width, Height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, FinalRenderTarget);
	CHECK_RESULT();
	return EErrorCode::OK;
#undef CHECK_RESULT
}

void FBlurMaterial::OnGui() noexcept
{
	ImGui::Begin("Blur");
	{
		ImGui::Checkbox("Is Enabled", &bIsEnabled);
		ImGui::SliderFloat("Smooth", &BlurParams.Smooth, 0, 1);
		ImGui::SliderFloat("Size", &BlurParams.Size, 0, 1);
		ImGui::SliderFloat2("Samples", static_cast<float*>(&BlurParams.SamplesX), 0.0f, 1.0f);
		ImGui::SliderFloat2("Direction", static_cast<float*>(&BlurParams.DirectionX), 0.0f, 1.0f);
		ImGui::SliderFloat2("Power", static_cast<float*>(&BlurParams.PowerX), 0.0f, 1.0f);
	}
	ImGui::End();
}

void FBlurMaterial::OnRender(const SRenderTarget* RenderTargets, const size_t Count)  noexcept
{
	assert(RenderTargets != nullptr);
	assert(Count == 1);

	if (bIsEnabled)
	{
		//X Pass
		InternalRenderer.SetRenderTarget(RenderTarget);
		InternalRenderer.ClearRenderTarget(RenderTarget, DirectX::XMFLOAT4(0.0f, 0.2f, 0.4f, 1.0f));
		InternalRenderer.SetViewport(RenderTarget.Width, RenderTarget.Height);
		InternalRenderer.SetShader(BlurXShader);
		InternalRenderer.SetConstantBuffer(BlurConstantBuffer, EShaderStage::PIXEL);
		InternalRenderer.UpdateSubresource(BlurConstantBuffer, &BlurParams, sizeof(SBlurParams));
		InternalRenderer.SetTexture(0, RenderTargets[0]);
		InternalRenderer.SetTexture(1, MaskTexture);
		InternalRenderer.SetConstantBuffer({ nullptr, 0 }, EShaderStage::VERTEX);
		InternalRenderer.SetVertexBuffer(0, { nullptr, 0 }, 0);
		InternalRenderer.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		InternalRenderer.Draw(3, 0);

		//Y Pass
		InternalRenderer.SetRenderTarget(FinalRenderTarget);
		InternalRenderer.ClearRenderTarget(FinalRenderTarget, DirectX::XMFLOAT4(0.0f, 0.2f, 0.4f, 1.0f));
		InternalRenderer.SetViewport(FinalRenderTarget.Width, FinalRenderTarget.Height);
		InternalRenderer.SetShader(BlurYShader);
		InternalRenderer.SetConstantBuffer(BlurConstantBuffer, EShaderStage::PIXEL);
		InternalRenderer.UpdateSubresource(BlurConstantBuffer, &BlurParams, sizeof(SBlurParams));
		InternalRenderer.SetTexture(0, RenderTarget);
		InternalRenderer.SetTexture(1, MaskTexture);
		InternalRenderer.SetConstantBuffer({ nullptr, 0 }, EShaderStage::VERTEX);
		InternalRenderer.SetVertexBuffer(0, { nullptr, 0 }, 0);
		InternalRenderer.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		InternalRenderer.Draw(3, 0);

		InternalRenderer.UnbindRenderTargets();
	}
}

void FBlurMaterial::SetMask(const SRenderTarget& Mask) noexcept
{
	MaskTexture = Mask;
}

const SRenderTarget& FBlurMaterial::GetResult() const noexcept
{
	return FinalRenderTarget;
}

bool FBlurMaterial::IsEnabled() const noexcept
{
	return bIsEnabled;
}
