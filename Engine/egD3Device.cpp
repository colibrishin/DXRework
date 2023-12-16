#include "pch.h"

#include "egD3Device.hpp"

#include "egDebugger.hpp"
#include "egShader.hpp"
#include "egToolkitAPI.h"

namespace Engine::Manager::Graphics
{
    void D3Device::GetSwapchainCopy(GraphicRenderedBuffer& buffer)
    {
        auto swapchain_texture = new ID3D11Texture2D*;
        auto buffer_texture    = new ID3D11Texture2D*;

        if (buffer.srv == nullptr)
        {
            ComPtr<ID3D11Texture2D> rtn_buffer;

            D3D11_TEXTURE2D_DESC desc{};

            desc.Width              = g_window_width;
            desc.Height             = g_window_height;
            desc.MipLevels          = 1;
            desc.ArraySize          = 1;
            desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count   = 1;
            desc.SampleDesc.Quality = 0;
            desc.Usage              = D3D11_USAGE_DEFAULT;
            desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags     = 0;
            desc.MiscFlags          = 0;

            DX::ThrowIfFailed(
                              m_device_->CreateTexture2D(&desc, nullptr, rtn_buffer.GetAddressOf()));

            D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};

            srv_desc.Format                         = desc.Format;
            srv_desc.ViewDimension                  = D3D11_SRV_DIMENSION_TEXTURE2D;
            srv_desc.Texture2D.MipLevels            = desc.MipLevels;
            srv_desc.Texture2D.MostDetailedMip      = 0;
            srv_desc.Texture2DArray.ArraySize       = 1;
            srv_desc.Texture2DArray.FirstArraySlice = 0;

            DX::ThrowIfFailed(
                              m_device_->CreateShaderResourceView(
                                                                  rtn_buffer.Get(), &srv_desc,
                                                                  buffer.srv.GetAddressOf()));
        }

        buffer.srv->GetResource(reinterpret_cast<ID3D11Resource**>(buffer_texture));
        m_swap_chain_->GetBuffer(
                                 0, __uuidof(ID3D11Texture2D),
                                 (void**)swapchain_texture);

        m_context_->CopyResource(*buffer_texture, *swapchain_texture);

        (*swapchain_texture)->Release();
        (*buffer_texture)->Release();

        delete swapchain_texture;
        delete buffer_texture;
    }

    void D3Device::UpdateBuffer(
        UINT          size, const void* data,
        ID3D11Buffer* buffer) const
    {
        D3D11_MAPPED_SUBRESOURCE mapped_resource{};

        DX::ThrowIfFailed(
                          m_context_->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource));
        memcpy(mapped_resource.pData, data, size);
        m_context_->Unmap(buffer, 0);
    }

    std::vector<D3D11_INPUT_ELEMENT_DESC> D3Device::GenerateInputDescription(
        Graphic::Shader<ID3D11VertexShader>* shader,
        ID3DBlob*                            blob)
    {
        ComPtr<ID3D11ShaderReflection> reflection = nullptr;

        D3DReflect(
                   blob->GetBufferPointer(), blob->GetBufferSize(),
                   IID_ID3D11ShaderReflection,
                   reinterpret_cast<void**>(reflection.GetAddressOf()));

        std::vector<D3D11_INPUT_ELEMENT_DESC> input_descs;

        D3D11_SHADER_DESC desc{};
        reflection->GetDesc(&desc);

        UINT byteOffset = 0;

        for (UINT i = 0; i < desc.InputParameters; ++i)
        {
            D3D11_SIGNATURE_PARAMETER_DESC param_desc;
            D3D11_INPUT_ELEMENT_DESC       input_desc{};
            reflection->GetInputParameterDesc(i, &param_desc);

            input_desc.SemanticName         = param_desc.SemanticName;
            input_desc.SemanticIndex        = param_desc.SemanticIndex;
            input_desc.InputSlot            = 0;
            input_desc.AlignedByteOffset    = byteOffset;
            input_desc.InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA;
            input_desc.InstanceDataStepRate = 0;

            // determine DXGI format
            if (param_desc.Mask == 1)
            {
                if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) input_desc.Format = DXGI_FORMAT_R32_UINT;
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                {
                    input_desc.Format = DXGI_FORMAT_R32_SINT;
                }
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                {
                    input_desc.Format = DXGI_FORMAT_R32_FLOAT;
                }
                byteOffset += 4;
            }
            else if (param_desc.Mask <= 3)
            {
                if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                {
                    input_desc.Format = DXGI_FORMAT_R32G32_UINT;
                }
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                {
                    input_desc.Format = DXGI_FORMAT_R32G32_SINT;
                }
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                {
                    input_desc.Format = DXGI_FORMAT_R32G32_FLOAT;
                }
                byteOffset += 8;
            }
            else if (param_desc.Mask <= 7)
            {
                if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                {
                    input_desc.Format = DXGI_FORMAT_R32G32B32_UINT;
                }
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                {
                    input_desc.Format = DXGI_FORMAT_R32G32B32_SINT;
                }
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                {
                    input_desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
                }
                byteOffset += 12;
            }
            else if (param_desc.Mask <= 15)
            {
                if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                {
                    input_desc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
                }
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                {
                    input_desc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
                }
                else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                {
                    input_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                }
                byteOffset += 16;
            }

            input_descs.push_back(input_desc);
        }

        return input_descs;
    }

    void D3Device::BindSampler(
        ID3D11SamplerState* sampler,
        eShaderType         target_shader,
        eSampler            sampler_type) const
    {
        g_shader_sampler_bind_map.at(target_shader)(
                                                    m_context_.Get(), sampler, static_cast<UINT>(sampler_type), 1);
    }

    void D3Device::CreateSampler(
        const D3D11_SAMPLER_DESC& desc,
        ID3D11SamplerState**      state) const
    {
        m_device_->CreateSamplerState(&desc, state);
    }

    void D3Device::CreateTexture(
        const D3D11_TEXTURE2D_DESC& desc,
        ID3D11Texture2D**           texture) const
    {
        DX::ThrowIfFailed(m_device_->CreateTexture2D(&desc, nullptr, texture));
    }

    void D3Device::CreateBlendState(ID3D11BlendState** blend_state) const
    {
        D3D11_BLEND_DESC desc{};
        desc.RenderTarget[0].BlendEnable           = true;
        desc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_MAX;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D10_COLOR_WRITE_ENABLE_ALL;

        m_device_->CreateBlendState(&desc, blend_state);
    }

    void D3Device::CreateDepthStencilState(
        ID3D11DepthStencilState** depth_stencil_state) const
    {
        D3D11_DEPTH_STENCIL_DESC depth_stencil_state_desc{};

        depth_stencil_state_desc.DepthEnable    = true;
        depth_stencil_state_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depth_stencil_state_desc.DepthFunc      = D3D11_COMPARISON_LESS_EQUAL;

        depth_stencil_state_desc.StencilEnable    = true;
        depth_stencil_state_desc.StencilReadMask  = 0xFF;
        depth_stencil_state_desc.StencilWriteMask = 0xFF;

        // Stencil operations if pixel is front-facing.
        depth_stencil_state_desc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
        depth_stencil_state_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
        depth_stencil_state_desc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
        depth_stencil_state_desc.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;

        // Stencil operations if pixel is back-facing.
        depth_stencil_state_desc.BackFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
        depth_stencil_state_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
        depth_stencil_state_desc.BackFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
        depth_stencil_state_desc.BackFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;

        DX::ThrowIfFailed(
                          m_device_->CreateDepthStencilState(
                                                             &depth_stencil_state_desc, depth_stencil_state));

        m_context_->OMSetDepthStencilState(*depth_stencil_state, 1);
    }

    void D3Device::InitializeDevice()
    {
        constexpr D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;

        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        ComPtr<ID3D11Device>        tmp_dev;
        ComPtr<ID3D11DeviceContext> tmp_ctx;

        DX::ThrowIfFailed(
                          D3D11CreateDevice(
                                            nullptr, D3D_DRIVER_TYPE_HARDWARE,
                                            nullptr, creationFlags, &feature_level, 1,
                                            D3D11_SDK_VERSION, tmp_dev.GetAddressOf(),
                                            nullptr, tmp_ctx.GetAddressOf()));

        DX::ThrowIfFailed(
                          tmp_dev->QueryInterface(IID_PPV_ARGS(m_device_.GetAddressOf())));
        DX::ThrowIfFailed(
                          tmp_ctx->QueryInterface(IID_PPV_ARGS(m_context_.GetAddressOf())));

        ComPtr<IDXGIFactory3> factory;
        ComPtr<IDXGIAdapter>  adapter;
        ComPtr<IDXGIDevice1>  dxgi_device;

        DX::ThrowIfFailed(
                          tmp_dev->QueryInterface(IID_PPV_ARGS(dxgi_device.GetAddressOf())));
        DX::ThrowIfFailed(dxgi_device->GetAdapter(&adapter));

        DX::ThrowIfFailed(adapter->GetParent(IID_PPV_ARGS(factory.GetAddressOf())));

        DXGI_ADAPTER_DESC adapter_desc;
        DX::ThrowIfFailed(adapter->GetDesc(&adapter_desc));

        s_video_card_memory_ =
                static_cast<UINT>(adapter_desc.DedicatedVideoMemory / 1024 / 1024);

        ComPtr<IDXGIOutput> monitor;
        UINT                mode_count = 0;

        DX::ThrowIfFailed(factory->EnumAdapters(0, &adapter));
        DX::ThrowIfFailed(adapter->EnumOutputs(0, &monitor));
        DX::ThrowIfFailed(
                          monitor->GetDisplayModeList(
                                                      DXGI_FORMAT_R8G8B8A8_UNORM,
                                                      DXGI_ENUM_MODES_INTERLACED,
                                                      &mode_count, nullptr));

        auto display_mode_list =
                std::unique_ptr<DXGI_MODE_DESC>(new DXGI_MODE_DESC[mode_count]);

        DX::ThrowIfFailed(
                          monitor->GetDisplayModeList(
                                                      DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED,
                                                      &mode_count,
                                                      display_mode_list.get()));

        for (UINT i = 0; i < mode_count; ++i)
        {
            if (display_mode_list.get()[i].Width == g_window_width &&
                display_mode_list.get()[i].Height == g_window_height)
            {
                s_refresh_rate_numerator_ =
                        display_mode_list.get()[i].RefreshRate.Numerator;
                s_refresh_rate_denominator_ =
                        display_mode_list.get()[i].RefreshRate.Denominator;
            }
        }

        DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};

        swap_chain_desc.BufferCount        = 2;
        swap_chain_desc.Width              = g_window_width;
        swap_chain_desc.Height             = g_window_height;
        swap_chain_desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
        swap_chain_desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc.SampleDesc.Count   = 1;
        swap_chain_desc.SampleDesc.Quality = 0;
        swap_chain_desc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
        swap_chain_desc.Scaling            = DXGI_SCALING_NONE;
        swap_chain_desc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swap_chain_desc.Flags              = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC full_screen_desc = {};

        full_screen_desc.Windowed         = !g_full_screen;
        full_screen_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

        if (g_vsync_enabled)
        {
            full_screen_desc.RefreshRate.Denominator = s_refresh_rate_denominator_;
            full_screen_desc.RefreshRate.Numerator   = s_refresh_rate_numerator_;
        }
        else
        {
            full_screen_desc.RefreshRate.Denominator = 1;
            full_screen_desc.RefreshRate.Numerator   = 0;
        }

        DX::ThrowIfFailed(
                          factory->CreateSwapChainForHwnd(
                                                          m_device_.Get(), m_hwnd_, &swap_chain_desc, &full_screen_desc,
                                                          nullptr,
                                                          (IDXGISwapChain1**)m_swap_chain_.GetAddressOf()));

        m_swap_chain_->SetMaximumFrameLatency(g_max_frame_latency_second);
    }

    void D3Device::InitializeRenderTargetView()
    {
        ComPtr<ID3D11Texture2D> back_buffer;

        DX::ThrowIfFailed(
                          m_swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), &back_buffer));
        DX::ThrowIfFailed(
                          m_device_->CreateRenderTargetView(
                                                            back_buffer.Get(), nullptr,
                                                            s_render_target_view_.ReleaseAndGetAddressOf()));
    }

    void D3Device::InitializeD2D()
    {
        ComPtr<ID2D1Factory> d2d_factory;
        DX::ThrowIfFailed(
                          D2D1CreateFactory(
                                            D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                            d2d_factory.GetAddressOf()));

        const float dpiX =
                static_cast<float>(GetDeviceCaps(GetDC(m_hwnd_), LOGPIXELSX));
        const float dpiY =
                static_cast<float>(GetDeviceCaps(GetDC(m_hwnd_), LOGPIXELSY));

        const D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
         D2D1_RENDER_TARGET_TYPE_DEFAULT,
         D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
         dpiX, dpiY);

        d2d_factory->CreateDxgiSurfaceRenderTarget(
                                                   m_surface_.Get(), &props, m_d2d_render_target_view_.GetAddressOf());
    }

    void D3Device::InitializeDepthStencil()
    {
        ComPtr<ID3D11Texture2D> depth_stencil_buffer;
        D3D11_TEXTURE2D_DESC    depth_stencil_desc{};

        depth_stencil_desc.Width              = g_window_width;
        depth_stencil_desc.Height             = g_window_height;
        depth_stencil_desc.MipLevels          = 1;
        depth_stencil_desc.ArraySize          = 1;
        depth_stencil_desc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depth_stencil_desc.SampleDesc.Count   = 1;
        depth_stencil_desc.SampleDesc.Quality = 0;
        depth_stencil_desc.Usage              = D3D11_USAGE_DEFAULT;
        depth_stencil_desc.BindFlags          = D3D11_BIND_DEPTH_STENCIL;
        depth_stencil_desc.CPUAccessFlags     = 0;
        depth_stencil_desc.MiscFlags          = 0;

        DX::ThrowIfFailed(
                          m_device_->CreateTexture2D(
                                                     &depth_stencil_desc, nullptr,
                                                     depth_stencil_buffer.ReleaseAndGetAddressOf()));

        D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc{};

        depth_stencil_view_desc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depth_stencil_view_desc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
        depth_stencil_view_desc.Texture2D.MipSlice = 0;

        DX::ThrowIfFailed(
                          m_device_->CreateDepthStencilView(
                                                            depth_stencil_buffer.Get(), &depth_stencil_view_desc,
                                                            s_depth_stencil_view_.ReleaseAndGetAddressOf()));

        m_context_->OMSetRenderTargets(
                                       1, s_render_target_view_.GetAddressOf(),
                                       s_depth_stencil_view_.Get());
    }

    void D3Device::CreateRasterizer(
        ID3D11RasterizerState** state,
        D3D11_FILL_MODE         fill_mode) const
    {
        D3D11_RASTERIZER_DESC rasterizer_desc{};

        rasterizer_desc.AntialiasedLineEnable = false;
        rasterizer_desc.CullMode              = D3D11_CULL_BACK;
        rasterizer_desc.DepthBias             = fill_mode == D3D11_FILL_WIREFRAME ? -1000 : 0;
        rasterizer_desc.DepthBiasClamp        =
                fill_mode == D3D11_FILL_WIREFRAME ? 0.0001f : 0.0f;
        rasterizer_desc.DepthClipEnable       = true;
        rasterizer_desc.FillMode              = fill_mode;
        rasterizer_desc.FrontCounterClockwise = false;
        rasterizer_desc.MultisampleEnable     = false;
        rasterizer_desc.ScissorEnable         = false;
        rasterizer_desc.SlopeScaledDepthBias  =
                fill_mode == D3D11_FILL_WIREFRAME ? 0.01f : 0.0f;

        DX::ThrowIfFailed(m_device_->CreateRasterizerState(&rasterizer_desc, state));
        m_context_->RSSetState(*state);
    }

    void D3Device::UpdateRenderTarget()
    {
        m_context_->OMSetRenderTargets(
                                       1, s_render_target_view_.GetAddressOf(),
                                       s_depth_stencil_view_.Get());
    }

    void D3Device::UpdateViewport()
    {
        s_viewport_ = {
            0.f,
            0.f,
            static_cast<float>(g_window_width),
            static_cast<float>(g_window_height),
            0.f,
            1.f
        };

        m_context_->RSSetViewports(1, &s_viewport_);
    }

    void D3Device::CreateTextureFromFile(
        const std::filesystem::path& path, ID3D11Resource** texture,
        ID3D11ShaderResourceView**   shader_resource_view) const
    {
        DX::ThrowIfFailed(
                          CreateDDSTextureFromFileEx(
                                                   m_device_.Get(), path.c_str(), 0, D3D11_USAGE_DEFAULT,
                                                   D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, 0,
                                                   D3D11_RESOURCE_MISC_GENERATE_MIPS, DDS_LOADER_DEFAULT, texture,
                                                   shader_resource_view));
    }

    void D3Device::PreUpdate(const float& dt) {}

    void D3Device::Update(const float& dt) {}

    void D3Device::PreRender(const float& dt)
    {
        FrameBegin();
    }

    void D3Device::Render(const float& dt) {}

    void D3Device::FixedUpdate(const float& dt) {}

    void D3Device::PostRender(const float& dt)
    {
        Present();
    }

    void D3Device::Initialize(HWND hwnd)
    {
        m_hwnd_ = hwnd;

        InitializeDevice();
        InitializeRenderTargetView();
        InitializeDepthStencil();
        UpdateViewport();

        m_projection_matrix_ = XMMatrixPerspectiveFovLH(
                                                        g_fov, GetAspectRatio(),
                                                        g_screen_near, g_screen_far);
        s_ortho_matrix_ = XMMatrixOrthographicLH(
                                                 static_cast<float>(g_window_width),
                                                 static_cast<float>(g_window_height),
                                                 g_screen_near, g_screen_far);
    }

    float D3Device::GetAspectRatio()
    {
        return static_cast<float>(g_window_width) /
               static_cast<float>(g_window_height);
    }

    void D3Device::FrameBegin()
    {
        if (WaitForSingleObjectEx(
                                  GetSwapchainAwaiter(), g_max_frame_latency_ms,
                                  true) != WAIT_OBJECT_0)
        {
            GetDebugger().Log(L"Waiting for Swap chain had an issue.");
        }

        constexpr float color[4] = {0.f, 0.f, 0.f, 1.f};

        m_context_->ClearRenderTargetView(s_render_target_view_.Get(), color);
        m_context_->ClearDepthStencilView(
                                          s_depth_stencil_view_.Get(),
                                          D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                                          1.f, 0);
    }

    void D3Device::Present() const
    {
        DXGI_PRESENT_PARAMETERS params{};
        params.DirtyRectsCount = 0;
        params.pDirtyRects     = nullptr;
        params.pScrollRect     = nullptr;
        params.pScrollOffset   = nullptr;

        m_swap_chain_->Present1(
                                g_vsync_enabled ? 1 : 0, DXGI_PRESENT_DO_NOT_WAIT,
                                &params);
    }
} // namespace Engine::Manager::Graphics
