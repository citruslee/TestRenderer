#pragma once
#include "Renderer.hpp"

class FLight
{
public:
	explicit FLight(FRenderer& Renderer);

	void Initialize(const uint32_t Width, const uint32_t Height) noexcept;

	void OnRender() noexcept;
	void OnUpdate(const float Time) noexcept;
	void OnGui() noexcept;
private:
	static constexpr uint8_t MAX_LIGHTS = 8;
	struct SLightData
	{
		DirectX::XMFLOAT4 Colour;
		DirectX::XMFLOAT3 Direction;
		float LightIntensity;
	};

	struct SLightConstantBuffer
	{
		SLightData Lights[MAX_LIGHTS];
		uint8_t LightCount;
	};

	SLightConstantBuffer LightConstantBuffer{};

	SBuffer ConstantBuffer{};
	FRenderer& InternalRenderer;
};