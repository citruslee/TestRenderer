#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#define WITH_EDITOR 1
#include "Application.hpp"
#include <chrono>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

uint32_t Width = 1600;
uint32_t Height = 900;

FApplication Application{};

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND HWnd, UINT Msg, WPARAM WParam, LPARAM LParam);

LRESULT CALLBACK WindowProc(const HWND HWnd, const UINT Message, const WPARAM WParam, const LPARAM LParam)
{
	if (ImGui_ImplWin32_WndProcHandler(HWnd, Message, WParam, LParam))
	{
		return true;
	}
	switch (Message)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	case WM_SIZE:
	{
		if (WParam != SIZE_MINIMIZED)
		{
			Width = static_cast<uint32_t>(LOWORD(LParam));
			Height = static_cast<uint32_t>(HIWORD(LParam));
			Application.Resize(Width, Height);
		}
	}
	default:
	{
		break;
	}
	}
	return DefWindowProc(HWnd, Message, WParam, LParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEX WindowClass{};
	WindowClass.cbSize = sizeof(WNDCLASSEX);
	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = WindowProc;
	WindowClass.hInstance = hInstance;
	WindowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	WindowClass.lpszClassName = "WindowClass";
	RegisterClassEx(&WindowClass);

	RECT WindowRect = { 0, 0, static_cast<LONG>(Width), static_cast<LONG>(Height)};
	AdjustWindowRect(&WindowRect, WS_OVERLAPPEDWINDOW, FALSE);

	const auto HWnd = CreateWindowEx(0, "WindowClass", "Test Renderer", WS_OVERLAPPEDWINDOW, 300, 300, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top, nullptr, nullptr, hInstance, nullptr);
	ShowWindow(HWnd, nShowCmd);
	UpdateWindow(HWnd);
	Application.Setup(HWnd, Width, Height);

	double Time = INFINITY;
	
	MSG Msg{};
	while (TRUE)
	{
		if (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);

			if (Msg.message == WM_QUIT)
			{
				break;
			}
		}
		
		auto& Io = ImGui::GetIO();

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		auto WindowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

		const auto Viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(Viewport->Pos);
		ImGui::SetNextWindowSize(Viewport->Size);
		ImGui::SetNextWindowViewport(Viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		WindowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
		WindowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		WindowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		bool Open = true;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace", &Open, WindowFlags);
		ImGui::PopStyleVar();
		ImGui::PopStyleVar(2);

		if (Io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			const auto DockspaceId = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(DockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
		}
		ImGui::End();

		// initialize time
		if (Time == INFINITY) {
			Time = Application.GetHighResolutionTime();
		}

		// calculate time difference
		const auto CurrentTime = Application.GetHighResolutionTime();
		const auto DeltaTime = CurrentTime - Time;
		Time = CurrentTime;

		// update application
		Application.OnGui();
		Application.OnUpdate(static_cast<float>(DeltaTime));
		Application.OnRender();

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		if (Io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		Application.Present();
		
	}

	Application.TearDown();
	
	return Msg.wParam;
}