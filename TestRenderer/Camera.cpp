#include "Camera.hpp"

#include "imgui/imgui.h"
#include <complex>

FCamera::FCamera(FRenderer& Renderer): InternalRenderer(Renderer)
{
}

FCamera::~FCamera()
{
	InternalRenderer.DestroyBuffer(ConstantBuffer);
}

void FCamera::Initialize(const uint32_t Width, const uint32_t Height) noexcept
{
	using namespace DirectX;
	
	Position = DirectX::XMVectorSet(0.0f, 5.0f, -10.0f, 0.0f);
	Target = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	View = DirectX::XMMatrixLookAtLH(Position, Target, Up);
	const auto AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
	Projection = DirectX::XMMatrixPerspectiveFovLH(0.4f * 3.14f, AspectRatio, 0.1f, 1000.0f);

	OnUpdate(0.0f);

	RotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(Pitch, Yaw, 0);
	Target = DirectX::XMVector3TransformCoord(DefaultForward, RotationMatrix);
	Target = DirectX::XMVector3Normalize(Target);

	Right = DirectX::XMVector3TransformCoord(DefaultRight, RotationMatrix);
	Forward = DirectX::XMVector3TransformCoord(DefaultForward, RotationMatrix);
	Up = DirectX::XMVector3Cross(Forward, Right);

	Position += LeftRight * Right;
	Position += BackForward * Forward;

	LeftRight = 0.0f;
	BackForward = 0.0f;

	Target = Position + Target;

	View = DirectX::XMMatrixLookAtLH(Position, Target, Up);

	DirectX::XMStoreFloat4(&CameraConstants.Position, Position);

	InternalRenderer.CreateConstantBufferWithData(CameraConstants, ConstantBuffer);
	
}

void FCamera::OnUpdate(const float Time) noexcept
{
	using namespace DirectX;
	
	auto& Io = ImGui::GetIO();
	if(!Io.WantCaptureKeyboard && !Io.WantCaptureMouse)
	{
		return;
	}
	
	if (ImGui::IsKeyDown('W'))
	{
		BackForward += 15.0f * Time * Speed;
	}
	if (ImGui::IsKeyDown('S'))
	{
		BackForward -= 15.0f * Time * Speed;
	}
	if (ImGui::IsKeyDown('A'))
	{
		LeftRight -= 15.0f * Time * Speed;
	}
	if (ImGui::IsKeyDown('D'))
	{
		LeftRight += 15.0f * Time * Speed;
	}
	if (ImGui::IsMouseDown(0))
	{
		Yaw += Io.MouseDelta.x * Time;
		Pitch += Io.MouseDelta.y * Time;
	}

	RotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(Pitch, Yaw, 0);
	Target = DirectX::XMVector3TransformCoord(DefaultForward, RotationMatrix);
	Target = DirectX::XMVector3Normalize(Target);

	Right = DirectX::XMVector3TransformCoord(DefaultRight, RotationMatrix);
	Forward = DirectX::XMVector3TransformCoord(DefaultForward, RotationMatrix);
	Up = DirectX::XMVector3Cross(Forward, Right);

	Position += LeftRight * Right;
	Position += BackForward * Forward;

	LeftRight = 0.0f;
	BackForward = 0.0f;

	Target = Position + Target;

	View = DirectX::XMMatrixLookAtLH(Position, Target, Up);
}

void FCamera::OnRender() noexcept
{
	DirectX::XMStoreFloat4(&CameraConstants.Position, Position);
	InternalRenderer.SetConstantBuffer(ConstantBuffer, EShaderStage::PIXEL);
	InternalRenderer.UpdateSubresource(ConstantBuffer, &CameraConstants, sizeof(SCameraConstants));
}

void FCamera::OnGui() noexcept
{
	ImGui::Begin("Camera");
	{
		DirectX::XMFLOAT4 TempVector;
		DirectX::XMStoreFloat4(&TempVector, Position);
		ImGui::DragFloat3("Position", &TempVector.x);
		ImGui::InputFloat("Speed Factor", &Speed);
	}
	ImGui::End();
}

DirectX::XMMATRIX FCamera::GetViewMatrix() const noexcept
{
	return View;
}

DirectX::XMMATRIX FCamera::GetProjectionMatrix() const noexcept
{
	return Projection;
}
