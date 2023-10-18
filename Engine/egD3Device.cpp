#include "pch.hpp"
#include "egD3Device.hpp"

#include "egToolkitAPI.hpp"

namespace Engine::Graphic
{
	std::vector<D3D11_INPUT_ELEMENT_DESC> D3Device::GenerateInputDescription(
		Shader<ID3D11VertexShader>* shader, ID3DBlob* blob)
	{
		ComPtr<ID3D11ShaderReflection> reflection = nullptr;
		const auto casted = dynamic_cast<VertexShader*>(shader);

		D3DReflect(blob->GetBufferPointer(), blob->GetBufferSize(), IID_ID3D11ShaderReflection,
		           reinterpret_cast<void**>(reflection.GetAddressOf()));

		std::vector<D3D11_INPUT_ELEMENT_DESC> input_descs;

		D3D11_SHADER_DESC desc{};
		reflection->GetDesc(&desc);

		UINT byteOffset = 0;

		for (UINT i = 0; i < desc.InputParameters; ++i)
		{
			D3D11_SIGNATURE_PARAMETER_DESC param_desc;
			D3D11_INPUT_ELEMENT_DESC input_desc{};
			reflection->GetInputParameterDesc(i, &param_desc);

			input_desc.SemanticName = param_desc.SemanticName;
			input_desc.SemanticIndex = param_desc.SemanticIndex;
			input_desc.InputSlot = 0;
			input_desc.AlignedByteOffset = byteOffset;
			input_desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			input_desc.InstanceDataStepRate = 0;

			// determine DXGI format
			if (param_desc.Mask == 1)
			{
				if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) input_desc.Format = DXGI_FORMAT_R32_UINT;
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) input_desc.Format =
					DXGI_FORMAT_R32_SINT;
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) input_desc.Format =
					DXGI_FORMAT_R32_FLOAT;
				byteOffset += 4;
			}
			else if (param_desc.Mask <= 3)
			{
				if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) input_desc.Format =
					DXGI_FORMAT_R32G32_UINT;
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) input_desc.Format =
					DXGI_FORMAT_R32G32_SINT;
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) input_desc.Format =
					DXGI_FORMAT_R32G32_FLOAT;
				byteOffset += 8;
			}
			else if (param_desc.Mask <= 7)
			{
				if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) input_desc.Format =
					DXGI_FORMAT_R32G32B32_UINT;
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) input_desc.Format =
					DXGI_FORMAT_R32G32B32_SINT;
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) input_desc.Format =
					DXGI_FORMAT_R32G32B32_FLOAT;
				byteOffset += 12;
			}
			else if (param_desc.Mask <= 15)
			{
				if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) input_desc.Format =
					DXGI_FORMAT_R32G32B32A32_UINT;
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) input_desc.Format =
					DXGI_FORMAT_R32G32B32A32_SINT;
				else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) input_desc.Format =
					DXGI_FORMAT_R32G32B32A32_FLOAT;
				byteOffset += 16;
			}

			input_descs.push_back(input_desc);
		}

		return input_descs;
	}

	void D3Device::CreateBlendState(ID3D11BlendState** blend_state)
	{
		D3D11_BLEND_DESC desc{};
		desc.RenderTarget[0].BlendEnable = true;
		desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_COLOR;
		desc.RenderTarget[0].DestBlend = D3D11_BLEND_BLEND_FACTOR;
		desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D10_COLOR_WRITE_ENABLE_ALL;

		s_device_->CreateBlendState(&desc, blend_state);
	}

	void D3Device::InitializeAdapter()
	{
		ComPtr<IDXGIFactory> factory;
		ComPtr<IDXGIAdapter> adapter;

		ComPtr<IDXGIOutput> monitor;
		UINT mode_count = 0;
		std::unique_ptr<DXGI_MODE_DESC> display_mode_list;

		DX::ThrowIfFailed(CreateDXGIFactory(__uuidof(IDXGIFactory), &factory));
		DX::ThrowIfFailed(factory->EnumAdapters(0, &adapter));
		DX::ThrowIfFailed(adapter->EnumOutputs(0, &monitor));
		DX::ThrowIfFailed(monitor->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED,
		                                              &mode_count, nullptr));

		display_mode_list = std::unique_ptr<DXGI_MODE_DESC>(new DXGI_MODE_DESC[mode_count]);

		DX::ThrowIfFailed(monitor->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED,
		                                              &mode_count, display_mode_list.get()));

		for (UINT i = 0; i < mode_count; ++i)
		{
			if (display_mode_list.get()[i].Width == g_window_width && display_mode_list.get()[i].Height ==
				g_window_height)
			{
				s_refresh_rate_numerator_ = display_mode_list.get()[i].RefreshRate.Numerator;
				s_refresh_rate_denominator_ = display_mode_list.get()[i].RefreshRate.Denominator;
			}
		}

		DXGI_ADAPTER_DESC adapter_desc;
		DX::ThrowIfFailed(adapter->GetDesc(&adapter_desc));

		s_video_card_memory_ = static_cast<UINT>(adapter_desc.DedicatedVideoMemory / 1024 / 1024);
	}

	void D3Device::InitializeDevice()
	{
		DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};

		swap_chain_desc.BufferCount = 1;
		swap_chain_desc.BufferDesc.Width = g_window_width;
		swap_chain_desc.BufferDesc.Height = g_window_height;
		swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swap_chain_desc.SampleDesc.Count = 1;
		swap_chain_desc.SampleDesc.Quality = 0;
		swap_chain_desc.OutputWindow = s_hwnd_;
		swap_chain_desc.Windowed = !g_full_screen;
		swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swap_chain_desc.Flags = 0;

		if (g_vsync_enabled)
		{
			swap_chain_desc.BufferDesc.RefreshRate.Numerator = s_refresh_rate_numerator_;
			swap_chain_desc.BufferDesc.RefreshRate.Denominator = s_refresh_rate_denominator_;
		}
		else
		{
			swap_chain_desc.BufferDesc.RefreshRate.Numerator = 0;
			swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
		}

		constexpr D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;

		UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		DX::ThrowIfFailed(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, creationFlags,
		                                                &feature_level, 1, D3D11_SDK_VERSION, &swap_chain_desc,
		                                                s_swap_chain_.GetAddressOf(), s_device_.GetAddressOf(), nullptr,
		                                                s_context_.GetAddressOf()));
	}

	void D3Device::InitializeRenderTargetView()
	{
		ComPtr<ID3D11Texture2D> back_buffer;

		DX::ThrowIfFailed(s_swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), &back_buffer));
		DX::ThrowIfFailed(
			s_device_->CreateRenderTargetView(back_buffer.Get(), nullptr,
			                                  s_render_target_view_.ReleaseAndGetAddressOf()));
	}

	void D3Device::InitializeDepthStencil()
	{
		D3D11_TEXTURE2D_DESC depth_stencil_desc{};

		depth_stencil_desc.Width = g_window_width;
		depth_stencil_desc.Height = g_window_height;
		depth_stencil_desc.MipLevels = 1;
		depth_stencil_desc.ArraySize = 1;
		depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depth_stencil_desc.SampleDesc.Count = 1;
		depth_stencil_desc.SampleDesc.Quality = 0;
		depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
		depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depth_stencil_desc.CPUAccessFlags = 0;
		depth_stencil_desc.MiscFlags = 0;

		DX::ThrowIfFailed(s_device_->CreateTexture2D(&depth_stencil_desc, nullptr,
		                                             s_depth_stencil_buffer_.ReleaseAndGetAddressOf()));

		D3D11_DEPTH_STENCIL_DESC depth_stencil_state_desc{};

		depth_stencil_state_desc.DepthEnable = true;
		depth_stencil_state_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depth_stencil_state_desc.DepthFunc = D3D11_COMPARISON_LESS;

		depth_stencil_state_desc.StencilEnable = true;
		depth_stencil_state_desc.StencilReadMask = 0xFF;
		depth_stencil_state_desc.StencilWriteMask = 0xFF;

		// Stencil operations if pixel is front-facing.
		depth_stencil_state_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_state_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
		depth_stencil_state_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_state_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		// Stencil operations if pixel is back-facing.
		depth_stencil_state_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_state_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
		depth_stencil_state_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_state_desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		DX::ThrowIfFailed(
			s_device_->CreateDepthStencilState(&depth_stencil_state_desc,
			                                   s_depth_stencil_state_.ReleaseAndGetAddressOf()));

		s_context_->OMSetDepthStencilState(s_depth_stencil_state_.Get(), 1);

		D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc{};

		depth_stencil_view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depth_stencil_view_desc.Texture2D.MipSlice = 0;

		DX::ThrowIfFailed(s_device_->CreateDepthStencilView(s_depth_stencil_buffer_.Get(), &depth_stencil_view_desc,
		                                                    s_depth_stencil_view_.ReleaseAndGetAddressOf()));

		s_context_->OMSetRenderTargets(1, s_render_target_view_.GetAddressOf(), s_depth_stencil_view_.Get());
	}

	void D3Device::CreateRasterizer(ID3D11RasterizerState** state)
	{
		D3D11_RASTERIZER_DESC rasterizer_desc{};

		rasterizer_desc.AntialiasedLineEnable = false;
		rasterizer_desc.CullMode = D3D11_CULL_BACK;
		rasterizer_desc.DepthBias = 0;
		rasterizer_desc.DepthBiasClamp = 0.0f;
		rasterizer_desc.DepthClipEnable = true;
		rasterizer_desc.FillMode = D3D11_FILL_SOLID;
		rasterizer_desc.FrontCounterClockwise = false;
		rasterizer_desc.MultisampleEnable = false;
		rasterizer_desc.ScissorEnable = false;
		rasterizer_desc.SlopeScaledDepthBias = 0.0f;

		DX::ThrowIfFailed(
			s_device_->CreateRasterizerState(&rasterizer_desc, s_rasterizer_state_.ReleaseAndGetAddressOf()));
		s_context_->RSSetState(s_rasterizer_state_.Get());
	}

	void D3Device::UpdateRenderTarget()
	{
		s_context_->OMSetRenderTargets(1, &s_render_target_view_, s_depth_stencil_view_.Get());
	}

	void D3Device::UpdateViewport()
	{
		s_viewport_ = {0.f, 0.f, static_cast<float>(g_window_width), static_cast<float>(g_window_height), 0.f, 1.f};

		s_context_->RSSetViewports(1, &s_viewport_);
	}

	void D3Device::CreateTextureFromFile(const std::filesystem::path& path, ID3D11Resource** texture,
	                                            ID3D11ShaderResourceView** shader_resource_view)
	{
		DX::ThrowIfFailed(CreateWICTextureFromFileEx(
			s_device_.Get(),
			path.c_str(),
			0,
			D3D11_USAGE_DEFAULT,
			D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
			0,
			D3D11_RESOURCE_MISC_GENERATE_MIPS,
			WIC_LOADER_DEFAULT,
			texture,
			shader_resource_view));
	}

	void D3Device::Initialize(HWND hwnd)
	{
		if (s_instance_)
		{
			return;
		}

		s_instance_ = std::unique_ptr<D3Device>(new D3Device);

		s_hwnd_ = hwnd;

		InitializeAdapter();
		InitializeDevice();
		InitializeRenderTargetView();
		InitializeDepthStencil();
		UpdateViewport();

		s_world_matrix_ = XMMatrixIdentity();
		s_projection_matrix_ = XMMatrixPerspectiveFovLH(XM_PI / 4.0f, GetAspectRatio(), g_screen_near, g_screen_far);
		s_ortho_matrix_ = XMMatrixOrthographicLH(g_window_width, g_window_height, g_screen_near, g_screen_far);

		RenderPipeline::Initialize();
		ToolkitAPI::Initialize();
	}

	void D3Device::FrameBegin()
	{
		float color[4] = {0.f, 0.f, 0.f, 1.f};

		s_context_->ClearRenderTargetView(s_render_target_view_.Get(), color);
		s_context_->ClearDepthStencilView(s_depth_stencil_view_.Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
	}

	void D3Device::Present()
	{
		s_swap_chain_->Present(g_vsync_enabled ? 1 : 0, DXGI_PRESENT_DO_NOT_WAIT);
	}
}
