#pragma once

#include "imgui/imgui.h"

#include <d3d11.h>

#include <DirectXMath.h>
#include <type_traits>
#include "ShaderStage.hpp"

enum class EErrorCode
{
	OK,
	FAIL,
	FILENOTFOUND,
	INVALIDCALL,
	NOTIMPLEMENTED
};

struct SBuffer
{
	ID3D11Buffer* Buffer;
	uint32_t Stride;
};

struct SRenderTarget
{
	mutable ID3D11RenderTargetView* RenderTargetView = nullptr;
	mutable ID3D11ShaderResourceView* ShaderResourceView = nullptr;
	mutable ID3D11DepthStencilView* DepthStencilView = nullptr;
	mutable ID3D11Texture2D* Texture = nullptr;
	mutable ID3D11Texture2D* DepthTexture = nullptr;
	mutable uint32_t Width = 0;
	mutable uint32_t Height = 0;
};

struct SShader
{
	ID3D11VertexShader* Vertex = nullptr;
	ID3D11PixelShader* Pixel = nullptr;
	ID3D11InputLayout* Layout = nullptr;
	EShaderStage Stage;
};

class FRenderer
{
public:
	explicit FRenderer() = default;
	~FRenderer();

	FRenderer(const FRenderer&) = delete;
	FRenderer(FRenderer&&) = delete;
	FRenderer& operator=(FRenderer&&) = delete;
	FRenderer& operator=(const FRenderer&) { return *this; }

	EErrorCode InitializeImGui() const noexcept;

	EErrorCode CreateDeviceAndSwapchainForHwnd(HWND WindowHandle, const size_t Width, const size_t Height, SRenderTarget& BackBuffer) noexcept;
	template <typename TType>
	EErrorCode CreateVertexBufferWithData(const TType* Data, const size_t Count, SBuffer& Buffer) const noexcept;
	EErrorCode CreateIndexBufferWithData(const uint32_t* Data, const size_t Count, SBuffer& Buffer) const noexcept;
	template <typename TType>
	EErrorCode CreateConstantBufferWithData(const TType& Data, SBuffer& Buffer) const noexcept;
	EErrorCode CreateVertexShader(const wchar_t* FileName, const char* EntryPoint, const D3D11_INPUT_ELEMENT_DESC* InputElementDescriptorArray, const size_t InputElementCount, SShader& Shader) const noexcept;
	EErrorCode CreatePixelShader(const wchar_t* FileName, const char* EntryPoint, SShader& Shader) const noexcept;
	EErrorCode CreateRenderTarget(const uint32_t Width, const uint32_t Height, const DXGI_FORMAT Format, SRenderTarget& RenderTarget) const noexcept;
	EErrorCode CreateDepthStencil(const uint32_t Width, const uint32_t Height, const DXGI_FORMAT Format, SRenderTarget& DepthStencil) const noexcept;
	EErrorCode CreateTextureFromFile(const char* FileName, const DXGI_FORMAT Format, SRenderTarget& Texture) const noexcept;
	EErrorCode CreateTextureFromMemory(const uint8_t* Data, const uint32_t Width, const uint32_t Height, const uint8_t Components, const DXGI_FORMAT Format, SRenderTarget& Texture) const noexcept;
	EErrorCode CreateCubeMapTexture(const char* Directory, SRenderTarget& CubeMap) const noexcept;

	void DestroyBuffer(SBuffer& Buffer) const noexcept;
	void DestroyShader(SShader& Shader) const noexcept;
	void DestroyRenderTarget(SRenderTarget& RenderTarget) const noexcept;
	void DestroyTexture(SRenderTarget& Texture) const noexcept;

	void ResizeBackBuffer(const uint32_t Width, const uint32_t Height, const SRenderTarget& BackBuffer) const noexcept;
	template <typename TType>
	void UpdateSubresource(const SBuffer& Buffer, const TType* Data, const size_t ByteSize)  const noexcept;

	void ClearRenderTarget(const SRenderTarget& RenderTarget, const DirectX::XMFLOAT4& Colour) const noexcept;
	void ClearDepthStencil(const SRenderTarget& RenderTarget, const uint32_t ClearFlags, const float Depth, const uint8_t Stencil) const noexcept;

	void UnbindRenderTargets() const noexcept;

	void SetConstantBuffer(const SBuffer& ConstantBuffer, const EShaderStage ShaderStage, const size_t Slot = 0) const noexcept;
	void SetViewport(const uint32_t Width, const uint32_t Height, const uint32_t XOffset = 0, const uint32_t YOffset = 0, const float MinDepth = 0.0f, const float MaxDepth = 1.0f) const noexcept;
	void SetShader(const SShader& Shader) const noexcept;
	void SetRenderTarget(const SRenderTarget& RenderTarget) const noexcept;
	void SetRenderTargets(const size_t Count, const SRenderTarget* RenderTarget) const noexcept;
	void SetTexture(const uint32_t Slot, const SRenderTarget& Texture) const noexcept;
	void SetPrimitiveTopology(const D3D11_PRIMITIVE_TOPOLOGY PrimitiveTopology) const noexcept;
	void SetVertexBuffer(const size_t StartSlot, const SBuffer& Buffer, const uint32_t Offset) const noexcept;
	void SetIndexBuffer(const size_t StartSlot, const SBuffer& Buffer, const uint32_t Offset) const noexcept;

	void Draw(const size_t VertexCount, const size_t VertexLocationStart) const noexcept;
	void DrawIndexed(const size_t IndexCount, const size_t IndexLocationStart = 0, const size_t VertexLocationBase = 0) const noexcept;

	EErrorCode Present(const size_t SyncInterval = 0, const size_t Flags = 0) const noexcept;

private:
	ID3D11Device* Device;
	ID3D11DeviceContext* DeviceContext;
	IDXGISwapChain* Swapchain;
	ID3D11SamplerState* LinearWrapSampler;
	ID3D11SamplerState* LinearClampSampler;
};

template <typename TType>
EErrorCode FRenderer::CreateVertexBufferWithData(const TType* Data, const size_t Count, SBuffer& Buffer) const noexcept
{
	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	BufferDesc.ByteWidth = sizeof(TType) * Count;
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	Buffer.Stride = sizeof(TType);

	D3D11_SUBRESOURCE_DATA InitialData{};
	InitialData.pSysMem = Data;
	InitialData.SysMemPitch = 0;
	InitialData.SysMemSlicePitch = 0;
	
	const auto HResult = Device->CreateBuffer(&BufferDesc, &InitialData, &Buffer.Buffer);
	if (HResult != S_OK)
	{
		return EErrorCode::FAIL;
	}

	return EErrorCode::OK;
}

template <typename TType>
EErrorCode FRenderer::CreateConstantBufferWithData(const TType& Data, SBuffer& Buffer) const noexcept
{
	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	BufferDesc.ByteWidth = (sizeof(TType) | 15) + 1;
	BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	const auto HResult = Device->CreateBuffer(&BufferDesc, nullptr, &Buffer.Buffer);
	if (HResult != S_OK)
	{
		return EErrorCode::FAIL;
	}

	UpdateSubresource(Buffer, &Data, sizeof(TType));
	return EErrorCode::OK;
}

template <typename TType>
void FRenderer::UpdateSubresource(const SBuffer& Buffer, const TType* Data, const size_t ByteSize) const noexcept
{
	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	DeviceContext->Map(Buffer.Buffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &MappedSubresource);
	memcpy(MappedSubresource.pData, Data, ByteSize);
	DeviceContext->Unmap(Buffer.Buffer, NULL);
}
