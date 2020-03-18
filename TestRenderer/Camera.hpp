#pragma once

#include <DirectXMath.h>
#include "imgui/imgui.h"
#include "Renderer.hpp"

class FCamera
{
public:
	explicit FCamera(FRenderer& Renderer);

	~FCamera();

	void Initialize(const uint32_t Width, const uint32_t Height) noexcept;
	void OnUpdate(const float Time) noexcept;
	void OnRender() noexcept;
	void OnGui() noexcept;

	DirectX::XMMATRIX GetViewMatrix() const noexcept;
	DirectX::XMMATRIX GetProjectionMatrix() const noexcept;

private:
	FRenderer& InternalRenderer;
	DirectX::XMVECTOR Position{};
	DirectX::XMVECTOR Target{};
	DirectX::XMVECTOR Up{};
	DirectX::XMVECTOR DefaultForward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	DirectX::XMVECTOR DefaultRight = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	DirectX::XMVECTOR Forward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	DirectX::XMVECTOR Right = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX RotationMatrix{};
	
	DirectX::XMMATRIX View{};
	DirectX::XMMATRIX Projection{};

	struct SCameraConstants
	{
		DirectX::XMFLOAT4 Position;
	};
	SCameraConstants CameraConstants{};
	SBuffer ConstantBuffer{};

	float Speed = 1.0f;

	float LeftRight = 0.0f;
	float BackForward = 0.0f;
	float Yaw = 0.0f;
	float Pitch = 0.0f;
};
