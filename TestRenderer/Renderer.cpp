#include "Renderer.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "imgui/imgui_impl_dx11.h"
#include <d3dcompiler.h>
#include <string>

FRenderer::~FRenderer()
{
	
	
	ID3D11Debug* d3dDebug = nullptr;
	if (SUCCEEDED(Device->QueryInterface(__uuidof(ID3D11Debug), (void**)& d3dDebug)))
	{
		ID3D11InfoQueue* d3dInfoQueue = nullptr;
		if (SUCCEEDED(d3dDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)& d3dInfoQueue)))
		{
#ifdef _DEBUG
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif

			D3D11_MESSAGE_ID hide[] =
			{
			D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
			// Add more message IDs here as needed
			};

			D3D11_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.NumIDs = _countof(hide);
			filter.DenyList.pIDList = hide;
			d3dInfoQueue->AddStorageFilterEntries(&filter);
			d3dInfoQueue->Release();
		}
	}

	LinearClampSampler->Release();
	LinearClampSampler = nullptr;
	LinearWrapSampler->Release();
	LinearWrapSampler = nullptr;
	Swapchain->Release();
	Swapchain = nullptr;
	Device->Release();
	Device = nullptr;

	DeviceContext->ClearState();
	DeviceContext->Flush();
	DeviceContext->Release();

#ifdef _DEBUG
	d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL);

	d3dDebug->Release();
#endif
}

EErrorCode FRenderer::InitializeImGui() const noexcept
{
	if (Device && DeviceContext)
	{
		ImGui_ImplDX11_Init(Device, DeviceContext);
		return EErrorCode::OK;
	}
	return EErrorCode::INVALIDCALL;
}

EErrorCode FRenderer::CreateDeviceAndSwapchainForHwnd(HWND WindowHandle, const size_t Width, const size_t Height, SRenderTarget& BackBuffer) noexcept
{
	DXGI_SWAP_CHAIN_DESC SwapChainDesc{};
	SwapChainDesc.BufferCount = 1;
	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	SwapChainDesc.BufferDesc.Width = Width;
	SwapChainDesc.BufferDesc.Height = Height;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.OutputWindow = WindowHandle;
	SwapChainDesc.SampleDesc.Count = 4;
	SwapChainDesc.Windowed = true;
	SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	UINT CreationFlags = 0;
#if defined(_DEBUG)
	CreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT HResult = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, CreationFlags, nullptr, 0, D3D11_SDK_VERSION, &SwapChainDesc, &Swapchain, &Device, nullptr, &DeviceContext);
	if (HResult != S_OK)
	{
		return EErrorCode::FAIL;
	}

	ID3D11Texture2D* TemporaryBackBuffer = nullptr;
	HResult = Swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(&TemporaryBackBuffer));
	if (HResult != S_OK)
	{
		TemporaryBackBuffer->Release();
		return EErrorCode::FAIL;
	}

	HResult = Device->CreateRenderTargetView(TemporaryBackBuffer, nullptr, &BackBuffer.RenderTargetView);
	TemporaryBackBuffer->Release();
	TemporaryBackBuffer = nullptr;
	if (HResult != S_OK)
	{
		return EErrorCode::FAIL;
	}

	BackBuffer.Width = Width;
	BackBuffer.Height = Height;

	// create clamp sampler
	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = -FLT_MAX;
	samplerDesc.MaxLOD = FLT_MAX;

	HResult = Device->CreateSamplerState(&samplerDesc, &LinearClampSampler);
	if (HResult != S_OK)
	{
		return EErrorCode::FAIL;
	}

	// create wrap sampler
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	HResult = Device->CreateSamplerState(&samplerDesc, &LinearWrapSampler);
	if (HResult != S_OK)
	{
		return EErrorCode::FAIL;
	}

	return EErrorCode::OK;
}

EErrorCode FRenderer::CreateIndexBufferWithData(const uint32_t* Data, const size_t Count, SBuffer& Buffer) const noexcept
{
	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	BufferDesc.ByteWidth = sizeof(uint32_t) * Count;
	BufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	Buffer.Stride = sizeof(uint32_t);

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

EErrorCode FRenderer::CreateVertexShader(const wchar_t* FileName, const char* EntryPoint, const D3D11_INPUT_ELEMENT_DESC* InputElementDescriptorArray, const size_t InputElementCount, SShader& Shader) const noexcept
{
	ID3DBlob* Blob = nullptr;

	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	flags |= D3DCOMPILE_DEBUG;
#endif

	auto HResult = D3DCompileFromFile(FileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, EntryPoint, "vs_5_0", flags, 0, &Blob, nullptr);
	if (HResult != S_OK)
	{
		return EErrorCode::FAIL;
	}
	
	HResult = Device->CreateVertexShader(Blob->GetBufferPointer(), Blob->GetBufferSize(), nullptr, &Shader.Vertex);
	if (HResult != S_OK)
	{
		return EErrorCode::FAIL;
	}
	Shader.Vertex->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(FileName) - 1, FileName);
	if (InputElementDescriptorArray)
	{
		HResult = Device->CreateInputLayout(InputElementDescriptorArray, InputElementCount, Blob->GetBufferPointer(), Blob->GetBufferSize(), &Shader.Layout);
		if (HResult != S_OK)
		{
			return EErrorCode::FAIL;
		}
	}
	Shader.Stage |= EShaderStage::VERTEX;
	Blob->Release();
	return EErrorCode::OK;
}

EErrorCode FRenderer::CreatePixelShader(const wchar_t* FileName, const char* EntryPoint, SShader& Shader) const noexcept
{
	ID3DBlob* Blob;

	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	flags |= D3DCOMPILE_DEBUG;
#endif
	
	auto HResult = D3DCompileFromFile(FileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, EntryPoint, "ps_5_0", flags, 0, &Blob, nullptr);
	if (HResult != S_OK)
	{
		return EErrorCode::FAIL;
	}
	
	HResult = Device->CreatePixelShader(Blob->GetBufferPointer(), Blob->GetBufferSize(), nullptr, &Shader.Pixel);
	if (HResult != S_OK)
	{
		return EErrorCode::FAIL;
	}
	Shader.Pixel->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(FileName) - 1, FileName);
	Shader.Stage |= EShaderStage::PIXEL;
	Blob->Release();
	return EErrorCode::OK;
}

EErrorCode FRenderer::CreateRenderTarget(const uint32_t Width, const uint32_t Height, const DXGI_FORMAT Format, SRenderTarget& RenderTarget) const noexcept
{
	D3D11_TEXTURE2D_DESC TextureDesc{};
	TextureDesc.Width = Width;
	TextureDesc.Height = Height;
	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 1;
	TextureDesc.Format = Format;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.MiscFlags = 0;
	auto HResult = Device->CreateTexture2D(&TextureDesc, nullptr, &RenderTarget.Texture);
	if (HResult != S_OK)
	{
		return EErrorCode::FAIL;
	}

	D3D11_RENDER_TARGET_VIEW_DESC RenderTargetViewDesc{};
	RenderTargetViewDesc.Format = TextureDesc.Format;
	RenderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	RenderTargetViewDesc.Texture2D.MipSlice = 0;
	HResult = Device->CreateRenderTargetView(RenderTarget.Texture, &RenderTargetViewDesc,  &RenderTarget.RenderTargetView);
	if (HResult != S_OK)
	{
		return EErrorCode::FAIL;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC ShaderResourceViewDesc{};
	ShaderResourceViewDesc.Format = TextureDesc.Format;
	ShaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	ShaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	ShaderResourceViewDesc.Texture2D.MipLevels = 1;
	HResult = Device->CreateShaderResourceView(RenderTarget.Texture, &ShaderResourceViewDesc, &RenderTarget.ShaderResourceView);
	if (HResult != S_OK)
	{
		return EErrorCode::FAIL;
	}

	RenderTarget.Width = Width;
	RenderTarget.Height = Height;

	return EErrorCode::OK;
}

EErrorCode FRenderer::CreateDepthStencil(const uint32_t Width, const uint32_t Height, const DXGI_FORMAT Format, SRenderTarget& DepthStencil) const noexcept
{
	D3D11_TEXTURE2D_DESC DepthStencilDesc{};
	DepthStencilDesc.Width = Width;
	DepthStencilDesc.Height = Height;
	DepthStencilDesc.MipLevels = 1;
	DepthStencilDesc.ArraySize = 1;
	DepthStencilDesc.Format = Format;
	DepthStencilDesc.SampleDesc.Count = 1;
	DepthStencilDesc.SampleDesc.Quality = 0;
	DepthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	DepthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	DepthStencilDesc.CPUAccessFlags = 0;
	DepthStencilDesc.MiscFlags = 0;

	auto HResult = Device->CreateTexture2D(&DepthStencilDesc, nullptr, &DepthStencil.DepthTexture);
	if (HResult != S_OK)
	{
		return EErrorCode::FAIL;
	}
	HResult = Device->CreateDepthStencilView(DepthStencil.DepthTexture, nullptr, &DepthStencil.DepthStencilView);
	if (HResult != S_OK)
	{
		return EErrorCode::FAIL;
	}
	return EErrorCode::OK;
}

EErrorCode FRenderer::CreateTextureFromFile(const char* FileName, const DXGI_FORMAT Format, SRenderTarget& Texture) const noexcept
{
	int ImageWidth = 0;
	int ImageHeight = 0;
	int Components = 0;
	uint8_t* ImageData = stbi_load(FileName, &ImageWidth, &ImageHeight, &Components, 4);
	if (ImageData == nullptr)
	{
		return EErrorCode::FAIL;
	}

	const auto Error = CreateTextureFromMemory(ImageData, ImageWidth, ImageHeight, 4, Format, Texture);
	if (Error != EErrorCode::OK)
	{
		return Error;
	}
	Texture.Width = ImageWidth;
	Texture.Height = ImageHeight;
	stbi_image_free(ImageData);

	return EErrorCode::OK;
}

EErrorCode FRenderer::CreateTextureFromMemory(const uint8_t* Data, const uint32_t Width, const uint32_t Height, const uint8_t Components, const DXGI_FORMAT Format, SRenderTarget& Texture) const noexcept
{
	D3D11_TEXTURE2D_DESC TextureDescriptor{};
	TextureDescriptor.Width = Width;
	TextureDescriptor.Height = Height;
	TextureDescriptor.MipLevels = 1;
	TextureDescriptor.ArraySize = 1;
	TextureDescriptor.Format = Format;
	TextureDescriptor.SampleDesc.Count = 1;
	TextureDescriptor.Usage = D3D11_USAGE_IMMUTABLE;
	TextureDescriptor.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	TextureDescriptor.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA Subresource{};
	Subresource.pSysMem = Data;
	Subresource.SysMemPitch = TextureDescriptor.Width * Components;
	Subresource.SysMemSlicePitch = 0;
	auto HResult = Device->CreateTexture2D(&TextureDescriptor, &Subresource, &Texture.Texture);
	if (HResult != S_OK)
	{
		return EErrorCode::FAIL;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = TextureDescriptor.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	Device->CreateShaderResourceView(Texture.Texture, &srvDesc, &Texture.ShaderResourceView);
	if (HResult != S_OK)
	{
		return EErrorCode::FAIL;
	}


	return EErrorCode::OK;
}

EErrorCode FRenderer::CreateCubeMapTexture(const char* Directory, SRenderTarget& CubeMap) const noexcept
{

	struct SCubeImageData
	{
		int ImageWidth = 0;
		int ImageHeight = 0;
		int Components = 0;
		std::string FileName;
		uint8_t* Pixels = nullptr;
	};

	SCubeImageData Images[6];

	Images[0].FileName = Directory;
	Images[0].FileName += "nx.png";

	Images[1].FileName = Directory;
	Images[1].FileName += "ny.png";

	Images[2].FileName = Directory;
	Images[2].FileName += "nz.png";

	Images[3].FileName = Directory;
	Images[3].FileName += "px.png";

	Images[4].FileName = Directory;
	Images[4].FileName += "py.png";

	Images[5].FileName = Directory;
	Images[5].FileName += "pz.png";
	
	Images[0].Pixels = stbi_load(Images[0].FileName.c_str(), &Images[0].ImageWidth, &Images[0].ImageHeight, &Images[0].Components, 4);
	Images[1].Pixels = stbi_load(Images[1].FileName.c_str(), &Images[1].ImageWidth, &Images[1].ImageHeight, &Images[1].Components, 4);
	Images[2].Pixels = stbi_load(Images[2].FileName.c_str(), &Images[2].ImageWidth, &Images[2].ImageHeight, &Images[2].Components, 4);
	Images[3].Pixels = stbi_load(Images[3].FileName.c_str(), &Images[3].ImageWidth, &Images[3].ImageHeight, &Images[3].Components, 4);
	Images[4].Pixels = stbi_load(Images[4].FileName.c_str(), &Images[4].ImageWidth, &Images[4].ImageHeight, &Images[4].Components, 4);
	Images[5].Pixels = stbi_load(Images[5].FileName.c_str(), &Images[5].ImageWidth, &Images[5].ImageHeight, &Images[5].Components, 4);

	for(size_t Index = 0; Index < 6; ++Index)
	{
		if(Images[Index].Pixels == nullptr)
		{
			return EErrorCode::FAIL;
		}
	}

	D3D11_TEXTURE2D_DESC TextureDesc;
	TextureDesc.Width = 2048;
	TextureDesc.Height = 2048;
	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 6;
	TextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	D3D11_SHADER_RESOURCE_VIEW_DESC SMViewDesc;
	SMViewDesc.Format = TextureDesc.Format;
	SMViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	SMViewDesc.TextureCube.MipLevels = TextureDesc.MipLevels;
	SMViewDesc.TextureCube.MostDetailedMip = 0;

	D3D11_SUBRESOURCE_DATA pData[6];
	for (size_t Index = 0; Index < 6; ++Index)
	{
		pData[Index].pSysMem = Images[Index].Pixels;
		pData[Index].SysMemPitch = Images[Index].Components;
		pData[Index].SysMemSlicePitch = 0;
	}
	
	HRESULT HResult = Device->CreateTexture2D(&TextureDesc, &pData[0], &CubeMap.Texture);
	if (HResult != S_OK)
	{
		stbi_image_free(Images[0].Pixels);
		stbi_image_free(Images[1].Pixels);
		stbi_image_free(Images[2].Pixels);
		stbi_image_free(Images[3].Pixels);
		stbi_image_free(Images[4].Pixels);
		stbi_image_free(Images[5].Pixels);
		return EErrorCode::FAIL;
	}
	HResult = Device->CreateShaderResourceView(CubeMap.Texture, &SMViewDesc, &CubeMap.ShaderResourceView);
	if (HResult != S_OK)
	{
		stbi_image_free(Images[0].Pixels);
		stbi_image_free(Images[1].Pixels);
		stbi_image_free(Images[2].Pixels);
		stbi_image_free(Images[3].Pixels);
		stbi_image_free(Images[4].Pixels);
		stbi_image_free(Images[5].Pixels);
		return EErrorCode::FAIL;
	}

	stbi_image_free(Images[0].Pixels);
	stbi_image_free(Images[1].Pixels);
	stbi_image_free(Images[2].Pixels);
	stbi_image_free(Images[3].Pixels);
	stbi_image_free(Images[4].Pixels);
	stbi_image_free(Images[5].Pixels);
	
	return EErrorCode::OK;
}

void FRenderer::DestroyRenderTarget(SRenderTarget& RenderTarget) const noexcept
{
	if (RenderTarget.RenderTargetView != nullptr)
	{
		RenderTarget.RenderTargetView->Release();
		RenderTarget.RenderTargetView = nullptr;
	}
	if (RenderTarget.ShaderResourceView != nullptr)
	{
		RenderTarget.ShaderResourceView->Release();
		RenderTarget.ShaderResourceView = nullptr;
	}
	if (RenderTarget.DepthStencilView != nullptr)
	{
		RenderTarget.DepthStencilView->Release();
		RenderTarget.DepthStencilView = nullptr;
	}
	if (RenderTarget.Texture != nullptr)
	{
		RenderTarget.Texture->Release();
		RenderTarget.Texture = nullptr;
	}
	if (RenderTarget.DepthTexture != nullptr)
	{
		RenderTarget.DepthTexture->Release();
		RenderTarget.DepthTexture = nullptr;
	}
}

void FRenderer::DestroyTexture(SRenderTarget& Texture) const noexcept
{
	if (Texture.ShaderResourceView != nullptr)
	{
		Texture.ShaderResourceView->Release();
		Texture.ShaderResourceView = nullptr;
	}
	if(Texture.Texture != nullptr)
	{
		Texture.Texture->Release();
		Texture.Texture = nullptr;
	}
}

void FRenderer::DestroyBuffer(SBuffer& Buffer) const noexcept
{
	if (Buffer.Buffer)
	{
		Buffer.Buffer->Release();
	}
}

void FRenderer::DestroyShader(SShader& Shader) const noexcept
{
	if (Shader.Vertex)
	{
		Shader.Vertex->Release();
	}
	if (Shader.Pixel)
	{
		Shader.Pixel->Release();
	}
	if (Shader.Layout)
	{
		Shader.Layout->Release();
	}
}

void FRenderer::ResizeBackBuffer(const uint32_t Width, const uint32_t Height, const SRenderTarget& BackBuffer) const noexcept
{
	if (!Device || !Swapchain)
	{
		return;
	}
	if (BackBuffer.RenderTargetView)
	{
		BackBuffer.RenderTargetView->Release();
		BackBuffer.RenderTargetView = nullptr;
	}

	Swapchain->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, 0);
	ID3D11Texture2D* TemporaryBackBuffer;
	Swapchain->GetBuffer(0, IID_PPV_ARGS(&TemporaryBackBuffer));
	Device->CreateRenderTargetView(TemporaryBackBuffer, nullptr, &BackBuffer.RenderTargetView);
	TemporaryBackBuffer->Release();

	BackBuffer.Width = Width;
	BackBuffer.Height = Height;
}

void FRenderer::ClearRenderTarget(const SRenderTarget& RenderTarget, const DirectX::XMFLOAT4& Colour) const noexcept
{
	DeviceContext->ClearRenderTargetView(RenderTarget.RenderTargetView, &Colour.x);
}

void FRenderer::ClearDepthStencil(const SRenderTarget& RenderTarget, const uint32_t ClearFlags, const float Depth, const uint8_t Stencil) const noexcept
{
	DeviceContext->ClearDepthStencilView(RenderTarget.DepthStencilView, ClearFlags, Depth, Stencil);
}

void FRenderer::UnbindRenderTargets() const noexcept
{
	ID3D11ShaderResourceView* NullSRViews[] = {
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr
	};
	ID3D11RenderTargetView* NullRTViews[] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

	DeviceContext->PSSetShaderResources(0, 16, NullSRViews);
	DeviceContext->OMSetRenderTargets(8, NullRTViews, nullptr);
}

void FRenderer::SetConstantBuffer(const SBuffer& ConstantBuffer, const EShaderStage ShaderStage, const size_t Slot) const noexcept
{
	if ((ShaderStage & EShaderStage::VERTEX) == EShaderStage::VERTEX)
	{
		DeviceContext->VSSetConstantBuffers(Slot, 1, &ConstantBuffer.Buffer);
	}

	if ((ShaderStage & EShaderStage::PIXEL) == EShaderStage::PIXEL)
	{
		DeviceContext->PSSetConstantBuffers(Slot, 1, &ConstantBuffer.Buffer);
	}
}

void FRenderer::SetViewport(const uint32_t Width, const uint32_t Height, const uint32_t XOffset, const uint32_t YOffset, const float MinDepth, const float MaxDepth) const noexcept
{
	D3D11_VIEWPORT Viewport{};
	Viewport.Width = static_cast<FLOAT>(Width);
	Viewport.Height = static_cast<FLOAT>(Height);
	Viewport.TopLeftX = static_cast<FLOAT>(XOffset);
	Viewport.TopLeftY = static_cast<FLOAT>(YOffset);
	Viewport.MinDepth = MinDepth;
	Viewport.MaxDepth = MaxDepth;

	DeviceContext->RSSetViewports(1, &Viewport);
}

void FRenderer::SetShader(const SShader& Shader) const noexcept
{
	if ((Shader.Stage & EShaderStage::VERTEX) == EShaderStage::VERTEX)
	{
		DeviceContext->VSSetShader(Shader.Vertex, nullptr, 0);
		DeviceContext->IASetInputLayout(Shader.Layout);
	}

	if ((Shader.Stage & EShaderStage::PIXEL) == EShaderStage::PIXEL)
	{
		DeviceContext->PSSetShader(Shader.Pixel, nullptr, 0);
	}
}

void FRenderer::SetRenderTarget(const SRenderTarget& RenderTarget) const noexcept
{
	DeviceContext->OMSetRenderTargets(1, &RenderTarget.RenderTargetView, RenderTarget.DepthStencilView);
}

void FRenderer::SetRenderTargets(const size_t Count, const SRenderTarget* RenderTarget) const noexcept
{
	ID3D11RenderTargetView* RenderTargetViewArray[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];

	for(size_t Index = 0; Index < Count; ++Index)
	{
		RenderTargetViewArray[Index] = RenderTarget[Index].RenderTargetView;
	}
	
	DeviceContext->OMSetRenderTargets(Count, RenderTargetViewArray, RenderTarget[0].DepthStencilView);
}

void FRenderer::SetTexture(const uint32_t Slot, const SRenderTarget& Texture) const noexcept
{
	ID3D11SamplerState *samplers[] = { LinearClampSampler, LinearWrapSampler };
	DeviceContext->PSSetShaderResources(Slot, 1, &Texture.ShaderResourceView);
	DeviceContext->PSSetSamplers(0, 2, samplers);
}

void FRenderer::SetPrimitiveTopology(const D3D11_PRIMITIVE_TOPOLOGY PrimitiveTopology) const noexcept
{
	DeviceContext->IASetPrimitiveTopology(PrimitiveTopology);
}

void FRenderer::SetVertexBuffer(const size_t StartSlot, const SBuffer& Buffer, const uint32_t Offset) const noexcept
{
	DeviceContext->IASetVertexBuffers(StartSlot, 1, &Buffer.Buffer, &Buffer.Stride, &Offset);
}

void FRenderer::SetIndexBuffer(const size_t StartSlot, const SBuffer& Buffer, const uint32_t Offset) const noexcept
{
	DeviceContext->IASetIndexBuffer(Buffer.Buffer, DXGI_FORMAT_R32_UINT, Offset);
}

void FRenderer::Draw(const size_t VertexCount, const size_t VertexLocationStart) const noexcept
{
	DeviceContext->Draw(VertexCount, VertexLocationStart);
}

void FRenderer::DrawIndexed(const size_t IndexCount, const size_t IndexLocationStart, const size_t VertexLocationBase) const noexcept
{
	DeviceContext->DrawIndexed(IndexCount, IndexLocationStart, VertexLocationBase);
}

EErrorCode FRenderer::Present(const size_t SyncInterval, const size_t Flags) const noexcept
{
	const auto HResult = Swapchain->Present(SyncInterval, Flags);
	if (HResult != S_OK)
	{
		return EErrorCode::FAIL;
	}
	return EErrorCode::OK;
}
