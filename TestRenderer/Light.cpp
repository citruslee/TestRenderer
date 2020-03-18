#include "Light.hpp"

FLight::FLight(FRenderer& Renderer): InternalRenderer(Renderer)
{
}

void FLight::Initialize(const uint32_t Width, const uint32_t Height) noexcept
{
	LightConstantBuffer.LightCount = 2;

	LightConstantBuffer.Lights[0].Direction = {-0.5f, -0.094f, -0.5f};
	LightConstantBuffer.Lights[0].Colour = {0.9f, 0.9f, 0.521f, 1.0f};
	LightConstantBuffer.Lights[0].LightIntensity = 1.5f;

	LightConstantBuffer.Lights[1].Direction = {-0.500f, -0.094f, 0.714f};
	LightConstantBuffer.Lights[1].Colour = {0.493f, 0.679f, 1.000f, 1.000f};
	LightConstantBuffer.Lights[1].LightIntensity = 5.0f;
	InternalRenderer.CreateConstantBufferWithData(LightConstantBuffer, ConstantBuffer);
}

void FLight::OnRender() noexcept
{
	InternalRenderer.SetConstantBuffer(ConstantBuffer, EShaderStage::PIXEL, 1);
	InternalRenderer.UpdateSubresource(ConstantBuffer, &LightConstantBuffer, sizeof(SLightConstantBuffer));
}

void FLight::OnUpdate(const float Time) noexcept
{
}

void FLight::OnGui() noexcept
{
	ImGui::Begin("Lights");
	{
		for (size_t Index = 0; Index < LightConstantBuffer.LightCount; ++Index)
		{
			ImGui::PushID(Index);
			ImGui::DragFloat3("Position", &LightConstantBuffer.Lights[Index].Direction.x, 0.001f, -1, 1);
			ImGui::DragFloat4("Colour", &LightConstantBuffer.Lights[Index].Colour.x, 0.001f, 0, 1);
			ImGui::DragFloat("Intensity", &LightConstantBuffer.Lights[Index].LightIntensity, 0.001f);
			ImGui::PopID();
			ImGui::Separator();
		}
		if (ImGui::Button("Push Light"))
		{
			if (LightConstantBuffer.LightCount < 8)
			{
				LightConstantBuffer.LightCount++;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Pop Light"))
		{
			if (LightConstantBuffer.LightCount > 0)
			{
				LightConstantBuffer.LightCount--;
			}
		}
	}
	ImGui::End();
}
