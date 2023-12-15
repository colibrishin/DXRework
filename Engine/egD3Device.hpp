#pragma once
#include <d2d1.h>
#include <d3dcompiler.h>
#include <filesystem>
#include "egCommon.hpp"
#include "egManager.hpp"
#include "egVertexShaderInternal.hpp"

namespace Engine::Manager::Graphics
{
    using namespace DirectX;
    using Microsoft::WRL::ComPtr;

    const std::unordered_map<
        eShaderType, std::function<void(
            ID3D11Device*, ID3D11DeviceContext*,
            ID3D11Buffer*, UINT, UINT)>>
    g_shader_cb_bind_map = {
        {
            SHADER_VERTEX,
            [](
        ID3D11Device* device, ID3D11DeviceContext* context,
        ID3D11Buffer* buffer, UINT                 start_slot, UINT num_buffers)
            {
                context->VSSetConstantBuffers(start_slot, num_buffers, &buffer);
            }
        },
        {
            SHADER_PIXEL,
            [](
        ID3D11Device* device, ID3D11DeviceContext* context,
        ID3D11Buffer* buffer, UINT                 start_slot, UINT num_buffers)
            {
                context->PSSetConstantBuffers(start_slot, num_buffers, &buffer);
            }
        },
        {
            SHADER_GEOMETRY,
            [](
        ID3D11Device* device, ID3D11DeviceContext* context,
        ID3D11Buffer* buffer, UINT                 start_slot, UINT num_buffers)
            {
                context->GSSetConstantBuffers(start_slot, num_buffers, &buffer);
            }
        },
        {
            SHADER_COMPUTE,
            [](
        ID3D11Device* device, ID3D11DeviceContext* context,
        ID3D11Buffer* buffer, UINT                 start_slot, UINT num_buffers)
            {
                context->CSSetConstantBuffers(start_slot, num_buffers, &buffer);
            }
        },
        {
            SHADER_HULL,
            [](
        ID3D11Device* device, ID3D11DeviceContext* context,
        ID3D11Buffer* buffer, UINT                 start_slot, UINT num_buffers)
            {
                context->HSSetConstantBuffers(start_slot, num_buffers, &buffer);
            }
        },
        {
            SHADER_DOMAIN,
            [](
        ID3D11Device* device, ID3D11DeviceContext* context,
        ID3D11Buffer* buffer, UINT                 start_slot, UINT num_buffers)
            {
                context->DSSetConstantBuffers(start_slot, num_buffers, &buffer);
            }
        }
    };

    const std::unordered_map<eShaderType,
                             std::function<void(
                                 ID3D11DeviceContext*,
                                 ID3D11SamplerState*, UINT, UINT)>>
    g_shader_sampler_bind_map = {
        {
            SHADER_VERTEX,
            [](
        ID3D11DeviceContext* context, ID3D11SamplerState* sampler,
        UINT                 start_slot, UINT             num_samplers)
            {
                context->VSSetSamplers(start_slot, num_samplers, &sampler);
            }
        },
        {
            SHADER_PIXEL,
            [](
        ID3D11DeviceContext* context, ID3D11SamplerState* sampler,
        UINT                 start_slot, UINT             num_samplers)
            {
                context->PSSetSamplers(start_slot, num_samplers, &sampler);
            }
        },
        {
            SHADER_GEOMETRY,
            [](
        ID3D11DeviceContext* context, ID3D11SamplerState* sampler,
        UINT                 start_slot, UINT             num_samplers)
            {
                context->GSSetSamplers(start_slot, num_samplers, &sampler);
            }
        },
        {
            SHADER_COMPUTE,
            [](
        ID3D11DeviceContext* context, ID3D11SamplerState* sampler,
        UINT                 start_slot, UINT             num_samplers)
            {
                context->CSSetSamplers(start_slot, num_samplers, &sampler);
            }
        },
        {
            SHADER_HULL,
            [](
        ID3D11DeviceContext* context, ID3D11SamplerState* sampler,
        UINT                 start_slot, UINT             num_samplers)
            {
                context->HSSetSamplers(start_slot, num_samplers, &sampler);
            }
        },
        {
            SHADER_DOMAIN,
            [](
        ID3D11DeviceContext* context, ID3D11SamplerState* sampler,
        UINT                 start_slot, UINT             num_samplers)
            {
                context->DSSetSamplers(start_slot, num_samplers, &sampler);
            }
        }
    };

    const std::map<GUID, eShaderType, GUIDComparer> g_shader_enum_type_map = {
        {__uuidof(ID3D11VertexShader), SHADER_VERTEX},
        {__uuidof(ID3D11PixelShader), SHADER_PIXEL},
        {__uuidof(ID3D11GeometryShader), SHADER_GEOMETRY},
        {__uuidof(ID3D11ComputeShader), SHADER_COMPUTE},
        {__uuidof(ID3D11HullShader), SHADER_HULL},
        {__uuidof(ID3D11DomainShader), SHADER_DOMAIN}
    };

    const std::unordered_map<std::wstring, eShaderType> g_shader_type_map = {
        {L"vs", SHADER_VERTEX}, {L"ps", SHADER_PIXEL}, {L"gs", SHADER_GEOMETRY},
        {L"cs", SHADER_COMPUTE}, {L"hs", SHADER_HULL}, {L"ds", SHADER_DOMAIN}
    };

    const std::unordered_map<eShaderType, std::string> g_shader_target_map = {
        {SHADER_VERTEX, "vs_5_0"}, {SHADER_PIXEL, "ps_5_0"},
        {SHADER_GEOMETRY, "gs_5_0"}, {SHADER_COMPUTE, "cs_5_0"},
        {SHADER_HULL, "hs_5_0"}, {SHADER_DOMAIN, "ds_5_0"}
    };

    class D3Device final : public Abstract::Singleton<D3Device, HWND>
    {
    public:
        D3Device(SINGLETON_LOCK_TOKEN)
        : Singleton() {}

        ~D3Device() override = default;

        void Initialize(HWND hWnd) override;

        static float GetAspectRatio();
        void         UpdateRenderTarget();
        void         UpdateViewport();

        template <typename T>
        void CreateBuffer(D3D11_BIND_FLAG flag, UINT size, ID3D11Buffer** buffer)
        {
            D3D11_BUFFER_DESC desc{};

            desc.Usage               = D3D11_USAGE_DYNAMIC;
            desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
            desc.BindFlags           = flag;
            desc.ByteWidth           = size * sizeof(T);
            desc.MiscFlags           = 0;
            desc.StructureByteStride = 0;

            DX::ThrowIfFailed(m_device_->CreateBuffer(&desc, nullptr, buffer));
        }

        template <typename T>
        void CreateBuffer(
            D3D11_BIND_FLAG flag, UINT size, ID3D11Buffer** buffer,
            void*           initial_data)
        {
            D3D11_BUFFER_DESC desc{};

            desc.Usage               = D3D11_USAGE_IMMUTABLE;
            desc.CPUAccessFlags      = 0;
            desc.BindFlags           = flag;
            desc.ByteWidth           = size * sizeof(T);
            desc.MiscFlags           = 0;
            desc.StructureByteStride = 0;

            D3D11_SUBRESOURCE_DATA data{};
            data.pSysMem = initial_data;

            DX::ThrowIfFailed(m_device_->CreateBuffer(&desc, &data, buffer));
        }

        void CreateTextureFromFile(
            const std::filesystem::path& path,
            ID3D11Resource**             texture,
            ID3D11ShaderResourceView**   shader_resource_view) const;

        template <typename T>
        void CreateShader(
            const std::filesystem::path& path,
            Graphic::Shader<T>*          shader)
        {
            ComPtr<ID3DBlob>  blob;
            ComPtr<ID3DBlob>  error;
            const eShaderType type = g_shader_enum_type_map.at(__uuidof(T));
            shader->Initialize();

            UINT flag = 0;

#if defined(_DEBUG)
            flag |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#endif

            D3DCompileFromFile(
                               path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                               "main", g_shader_target_map.at(type).c_str(), flag, 0,
                               &blob, &error);

            if (error)
            {
                std::string error_message =
                        static_cast<char*>(error->GetBufferPointer());
                OutputDebugStringA(error_message.c_str());
            }

            if constexpr (std::is_same_v<T, ID3D11VertexShader>)
            {
                const auto input_descs = GenerateInputDescription(shader, blob.Get());
                const auto casted      = dynamic_cast<Graphic::VertexShaderInternal*>(shader);

                DX::ThrowIfFailed(
                                  m_device_->CreateInputLayout(
                                                               input_descs.data(), static_cast<UINT>(input_descs.size()),
                                                               blob->GetBufferPointer(),
                                                               blob->GetBufferSize(), casted->GetInputLayout()));
                DX::ThrowIfFailed(
                                  m_device_->CreateVertexShader(
                                                                blob->GetBufferPointer(), blob->GetBufferSize(),
                                                                nullptr,
                                                                shader->GetShader()));
            }
            else if constexpr (std::is_same_v<T, ID3D11PixelShader>)
            {
                DX::ThrowIfFailed(
                                  m_device_->CreatePixelShader(
                                                               blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
                                                               shader->GetShader()));
            }
            else if constexpr (std::is_same_v<T, ID3D11GeometryShader>)
            {
                DX::ThrowIfFailed(
                                  m_device_->CreateGeometryShader(
                                                                  blob->GetBufferPointer(), blob->GetBufferSize(),
                                                                  nullptr,
                                                                  shader->GetShader()));
            }
            else if constexpr (std::is_same_v<T, ID3D11ComputeShader>)
            {
                DX::ThrowIfFailed(
                                  m_device_->CreateComputeShader(
                                                                 blob->GetBufferPointer(), blob->GetBufferSize(),
                                                                 nullptr,
                                                                 shader->GetShader()));
            }
            else if constexpr (std::is_same_v<T, ID3D11HullShader>)
            {
                DX::ThrowIfFailed(
                                  m_device_->CreateHullShader(
                                                              blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
                                                              shader->GetShader()));
            }
            else if constexpr (std::is_same_v<T, ID3D11DomainShader>)
            {
                DX::ThrowIfFailed(
                                  m_device_->CreateDomainShader(
                                                                blob->GetBufferPointer(), blob->GetBufferSize(),
                                                                nullptr,
                                                                shader->GetShader()));
            }
        }

        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PostRender(const float& dt) override;

        const Matrix& GetProjectionMatrix() const
        {
            return m_projection_matrix_;
        }

        const Matrix& GetOrthogonalMatrix() const
        {
            return s_ortho_matrix_;
        }

        ID3D11Device* GetDevice() const
        {
            return m_device_.Get();
        }

        ID3D11DeviceContext* GetContext() const
        {
            return m_context_.Get();
        }

        void GetSwapchainCopy(GraphicRenderedBuffer& buffer);

        HANDLE GetSwapchainAwaiter() const
        {
            return m_swap_chain_->GetFrameLatencyWaitableObject();
        }

    private:
        friend class RenderPipeline;
        friend class ToolkitAPI;

        D3Device() = default;

        void UpdateBuffer(UINT size, const void* data, ID3D11Buffer* buffer) const;

        template <typename T>
        void CreateConstantBuffer(ConstantBuffer<T>& buffer)
        {
            buffer.Create(m_device_.Get());
        }

        template <typename T>
        void BindConstantBuffer(
            ConstantBuffer<T>& buffer, eCBType type,
            eShaderType        target_shader)
        {
            g_shader_cb_bind_map.at(target_shader)(
                                                   m_device_.Get(), m_context_.Get(),
                                                   buffer.GetBuffer(), type, 1);
        }

        std::vector<D3D11_INPUT_ELEMENT_DESC> GenerateInputDescription(
            Graphic::Shader<ID3D11VertexShader>* shader,
            ID3DBlob*                            blob);

        template <typename T>
        void BindShader(Graphic::Shader<T>* shader)
        {
            if constexpr (std::is_same_v<T, ID3D11VertexShader>)
            {
                const auto casting = static_cast<Graphic::VertexShaderInternal*>(shader);
                m_context_->VSSetShader(*(casting->GetShader()), nullptr, 0);
                m_context_->IASetInputLayout(*casting->GetInputLayout());
            }
            else if constexpr (std::is_same_v<T, ID3D11PixelShader>)
            {
                m_context_->PSSetShader(*(shader->GetShader()), nullptr, 0);
            }
            else if constexpr (std::is_same_v<T, ID3D11GeometryShader>)
            {
                m_context_->GSSetShader(*(shader->GetShader()), nullptr, 0);
            }
            else if constexpr (std::is_same_v<T, ID3D11ComputeShader>)
            {
                m_context_->CSSetShader(*(shader->GetShader()), nullptr, 0);
            }
            else if constexpr (std::is_same_v<T, ID3D11HullShader>)
            {
                m_context_->HSSetShader(*(shader->GetShader()), nullptr, 0);
            }
            else if constexpr (std::is_same_v<T, ID3D11DomainShader>)
            {
                m_context_->DSSetShader(*(shader->GetShader()), nullptr, 0);
            }
        }

        void BindSampler(
            ID3D11SamplerState* sampler, eShaderType target_shader,
            eSampler            sampler_type) const;
        void CreateSampler(
            const D3D11_SAMPLER_DESC& desc,
            ID3D11SamplerState**      state) const;

        void CreateTexture(
            const D3D11_TEXTURE2D_DESC& desc,
            ID3D11Texture2D**           texture) const;
        void CreateBlendState(ID3D11BlendState** blend_state) const;
        void CreateDepthStencilState(ID3D11DepthStencilState** depth_stencil_state) const;
        void CreateRasterizer(
            ID3D11RasterizerState** state,
            D3D11_FILL_MODE         fill_mode) const;

    private:
        void InitializeDevice();
        void InitializeRenderTargetView();
        void InitializeD2D();
        void InitializeDepthStencil();

        void FrameBegin();
        void Present() const;

    private:
        HWND m_hwnd_ = nullptr;

        ComPtr<ID3D11Device1>        m_device_  = nullptr;
        ComPtr<ID3D11DeviceContext1> m_context_ = nullptr;

        ComPtr<IDXGISurface1>     m_surface_                = nullptr;
        ComPtr<ID2D1RenderTarget> m_d2d_render_target_view_ = nullptr;

        UINT s_video_card_memory_        = 0;
        UINT s_refresh_rate_numerator_   = 0;
        UINT s_refresh_rate_denominator_ = 0;

        DXGI_ADAPTER_DESC s_video_card_desc_ = {};

        ComPtr<IDXGISwapChain2>        m_swap_chain_         = nullptr;
        ComPtr<ID3D11RenderTargetView> s_render_target_view_ = nullptr;
        ComPtr<ID3D11DepthStencilView> s_depth_stencil_view_ = nullptr;

        D3D11_VIEWPORT s_viewport_{};

        XMMATRIX s_world_matrix_      = {};
        Matrix   m_projection_matrix_ = {};
        XMMATRIX s_ortho_matrix_      = {};
    };
} // namespace Engine::Manager::Graphics
