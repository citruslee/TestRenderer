#include "Application.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include "Renderer.hpp"
#include <DirectXMath.h>
#include <chrono>

EErrorCode FApplication::Setup(const HWND HWnd, const uint32_t Width, const uint32_t Height)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& Io = ImGui::GetIO();
	Io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	Io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	Io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	if (Io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGuiStyle& Style = ImGui::GetStyle();
		Style.WindowRounding = 0.0f;
		Style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(HWnd);
	
	auto Result = Renderer.CreateDeviceAndSwapchainForHwnd(HWnd, Width, Height, BackBuffer);

	Model.Initialize("Mesh/radio/Auna_Radio.obj", Width, Height);

	Result = Blur.Initialize(Width, Height);

	Renderer.CreateRenderTarget(Width, Height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, SceneRenderTarget);
	Renderer.CreateDepthStencil(Width, Height, DXGI_FORMAT_D24_UNORM_S8_UINT, SceneRenderTarget);

	MainCamera.Initialize(Width, Height);
	
	Result = Renderer.InitializeImGui();
	Light.Initialize(Width, Height);

	TexGen.Initialize(Width, Height);
	
	return EErrorCode::OK;
}

void FApplication::OnRender() noexcept
{
	Renderer.SetRenderTarget(SceneRenderTarget);
	Renderer.ClearRenderTarget(SceneRenderTarget, DirectX::XMFLOAT4(0.0f, 0.2f, 0.4f, 1.0f));
	Renderer.ClearDepthStencil(SceneRenderTarget, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	Renderer.SetViewport(SceneRenderTarget.Width, SceneRenderTarget.Height);
	Light.OnRender();
	Model.OnRender(nullptr, 0);

	Blur.OnRender(&SceneRenderTarget, 1);

	Renderer.SetRenderTarget(BackBuffer);
	Renderer.ClearRenderTarget(BackBuffer, DirectX::XMFLOAT4(0.0f, 0.2f, 0.4f, 1.0f));
}

void FApplication::OnUpdate(const float Time) noexcept
{
	MainCamera.OnUpdate(Time);
	Model.OnUpdate(Time);
	Light.OnUpdate(Time);
	TexGen.OnUpdate(Time);
	if (TexGen.IsOutputUpdated())
	{
		Blur.SetMask(TexGen.GetOutput());
	}
}

void FApplication::OnGui() noexcept
{
	Blur.OnGui();
	Model.OnGui();
	Light.OnGui();

	ImGui::Begin("Render Window");
	{
		auto& Io = ImGui::GetIO();
		if(ImGui::IsWindowHovered())
		{
			Io.WantCaptureMouse = Io.WantCaptureKeyboard = true;
		}
		else
		{
			Io.WantCaptureMouse = Io.WantCaptureKeyboard = false;
		}
		auto RenderTarget = Blur.GetResult();
		const float AspectRatio = static_cast<float>(RenderTarget.Width) / static_cast<float>(RenderTarget.Height);
		const auto WindowSize = ImGui::GetWindowSize();
		auto CursorPosition = ImGui::GetCursorPos();
		auto RenderSize = WindowSize;

		RenderSize.x -= CursorPosition.x * 2;
		RenderSize.y -= CursorPosition.y * 2;

		if (RenderSize.x < RenderSize.y * AspectRatio)
		{
			RenderSize.y = RenderSize.x / AspectRatio;
			CursorPosition.y = (WindowSize.y - RenderSize.y) * 0.5f;
		}
		else
		{
			RenderSize.x = RenderSize.y * AspectRatio;
			CursorPosition.x = (WindowSize.x - RenderSize.x) * 0.5f;
		}
		ImGui::SetCursorPos(CursorPosition);
		ImGui::Image(Blur.IsEnabled() ? RenderTarget.ShaderResourceView : SceneRenderTarget.ShaderResourceView, RenderSize);
	}
	ImGui::End();

	TexGen.OnGui();
}

void FApplication::TearDown() noexcept
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	Renderer.DestroyRenderTarget(BackBuffer);
	Renderer.DestroyRenderTarget(SceneRenderTarget);
}

void FApplication::Resize(const uint32_t Width, const uint32_t Height) const noexcept
{
	Renderer.ResizeBackBuffer(Width, Height, BackBuffer);
}

EErrorCode FApplication::Present() const noexcept
{
	return Renderer.Present(0, 0);
}

double FApplication::GetHighResolutionTime() noexcept
{
	static const std::chrono::high_resolution_clock::time_point Start = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point Now = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> TimeSpan = std::chrono::duration_cast<std::chrono::duration<double>>(Now - Start);
	return TimeSpan.count();
}
