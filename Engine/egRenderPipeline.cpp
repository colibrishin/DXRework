#include "pch.h"
#include "egRenderPipeline.h"

#include <filesystem>

#include "egD3Device.hpp"
#include "egManagerHelper.hpp"
#include "egToolkitAPI.h"

#include "egIShader.h"
#include "egShader.hpp"
#include "egVertexShaderInternal.h"
#include "egType.h"

namespace Engine::Manager::Graphics
{
    using namespace Engine::Resources;

    void RenderPipeline::SetWorldMatrix(const CBs::TransformCB& matrix)
    {
        m_transform_buffer_data_.SetData(GetD3Device().GetContext(), matrix);
        BindConstantBuffer(m_transform_buffer_data_, SHADER_VERTEX);
    }

    void RenderPipeline::SetPerspectiveMatrix(const CBs::PerspectiveCB& matrix)
    {
        m_wvp_buffer_data_.SetData(GetD3Device().GetContext(), matrix);
        BindConstantBuffer(m_wvp_buffer_data_, SHADER_VERTEX);
        BindConstantBuffer(m_wvp_buffer_data_, SHADER_PIXEL);
    }

    void RenderPipeline::SetLight(const CBs::LightCB& light)
    {
        m_light_buffer_data.SetData(GetD3Device().GetContext(), light);
        BindConstantBuffer(m_light_buffer_data, SHADER_VERTEX);
        BindConstantBuffer(m_light_buffer_data, SHADER_PIXEL);
        BindConstantBuffer(m_light_buffer_data, SHADER_GEOMETRY);
    }

    void RenderPipeline::SetCascadeBuffer(const CBs::ShadowVPCB& shadow_buffer)
    {
        m_shadow_buffer_data_.SetData(GetD3Device().GetContext(), shadow_buffer);
        BindConstantBuffer(m_shadow_buffer_data_, SHADER_GEOMETRY);
        BindConstantBuffer(m_shadow_buffer_data_, SHADER_PIXEL);
    }

    void RenderPipeline::SetTopology(const D3D11_PRIMITIVE_TOPOLOGY& topology)
    {
        GetD3Device().GetContext()->IASetPrimitiveTopology(topology);
    }

    void RenderPipeline::DefaultRenderTarget()
    {
        GetD3Device().GetContext()->OMSetDepthStencilState(
                                                           m_depth_stencil_state_.Get(), 0);
        GetD3Device().UpdateRenderTarget();
    }

    void RenderPipeline::DefaultViewport()
    {
        GetD3Device().UpdateViewport();
    }

    void RenderPipeline::SetWireframeState() const
    {
        GetD3Device().GetContext()->RSSetState(
                                               GetToolkitAPI().GetCommonStates()->Wireframe());
    }

    void RenderPipeline::SetFillState() const
    {
        GetD3Device().GetContext()->RSSetState(m_rasterizer_state_.Get());
    }

    void RenderPipeline::SetNoneCullState() const
    {
        GetD3Device().GetContext()->RSSetState(
                                               GetToolkitAPI().GetCommonStates()->CullNone());
    }

    void RenderPipeline::SetFrontCullState() const
    {
        GetD3Device().GetContext()->RSSetState(
                                               GetToolkitAPI().GetCommonStates()->CullClockwise());
    }

    void RenderPipeline::BindVertexBuffer(ID3D11Buffer* buffer)
    {
        constexpr UINT stride = sizeof(Graphics::VertexElement);
        constexpr UINT offset = 0;
        GetD3Device().GetContext()->IASetVertexBuffers(
                                                       0, 1, &buffer, &stride,
                                                       &offset);
    }

    void RenderPipeline::BindIndexBuffer(ID3D11Buffer* buffer)
    {
        GetD3Device().GetContext()->IASetIndexBuffer(buffer, DXGI_FORMAT_R32_UINT, 0);
    }

    void RenderPipeline::UnbindVertexBuffer()
    {
        constexpr UINT stride = 0;
        constexpr UINT offset = 0;
        ID3D11Buffer* null_buffer = nullptr;
        GetD3Device().GetContext()->IASetVertexBuffers(
                                                       0, 1, &null_buffer, &stride,
                                                       &offset);
    }

    void RenderPipeline::UnbindIndexBuffer()
    {
        ID3D11Buffer* null_buffer = nullptr;
        GetD3Device().GetContext()->IASetIndexBuffer(null_buffer, DXGI_FORMAT_R32_UINT, 0);
    }

    void RenderPipeline::BindResource(
        UINT                      slot,
        eShaderType               shader_type,
        ID3D11ShaderResourceView** texture)
    {
        g_shader_rs_bind_map.at(shader_type)(GetD3Device().GetContext(), texture,  slot, 1);
    }

    RenderPipeline::~RenderPipeline()
    {
        ResetShaders();
    }

    void RenderPipeline::Initialize()
    {
        GetD3Device().CreateConstantBuffer(m_wvp_buffer_data_);
        GetD3Device().CreateConstantBuffer(m_transform_buffer_data_);
        GetD3Device().CreateConstantBuffer(m_light_buffer_data);
        GetD3Device().CreateConstantBuffer(m_shadow_buffer_data_);
        GetD3Device().CreateConstantBuffer(m_shadow_buffer_chunk_data_);
        GetD3Device().CreateConstantBuffer(m_material_buffer_data_);

        PrecompileShaders();
        InitializeSamplers();

        GetD3Device().CreateBlendState(m_blend_state_.GetAddressOf());
        GetD3Device().CreateRasterizer(
                                       m_rasterizer_state_.GetAddressOf(),
                                       D3D11_FILL_SOLID);
        GetD3Device().CreateDepthStencilState(m_depth_stencil_state_.GetAddressOf());
    }

    void RenderPipeline::SetShader(Graphics::IShader* shader)
    {
        switch (shader->GetType())
        {
        case SHADER_VERTEX: GetD3Device().BindShader(reinterpret_cast<Resources::VertexShader*>(shader));
            break;
        case SHADER_PIXEL: GetD3Device().BindShader(reinterpret_cast<Resources::PixelShader*>(shader));
            break;
        case SHADER_GEOMETRY: GetD3Device().BindShader(
                                                       reinterpret_cast<Resources::GeometryShader*>(shader));
            break;
        case SHADER_COMPUTE: GetD3Device().BindShader(
                                                      reinterpret_cast<Resources::ComputeShader*>(shader));
            break;
        case SHADER_HULL: GetD3Device().BindShader(reinterpret_cast<Resources::HullShader*>(shader));
            break;
        case SHADER_DOMAIN: GetD3Device().BindShader(reinterpret_cast<Resources::DomainShader*>(shader));
            break;
        default:
            assert(nullptr);
        }
    }

    void RenderPipeline::UnbindShader(const Graphics::IShader* shader)
    {
        switch (shader->GetType())
        {
            case SHADER_VERTEX: GetD3Device().UnbindShader<VertexShader>();
                break;
            case SHADER_PIXEL: GetD3Device().UnbindShader<PixelShader>();
                break;
            case SHADER_GEOMETRY: GetD3Device().UnbindShader<GeometryShader>();
                break;
            case SHADER_COMPUTE: GetD3Device().UnbindShader<ComputeShader>();
                break;
            case SHADER_HULL: GetD3Device().UnbindShader<HullShader>();
                break;
            case SHADER_DOMAIN: GetD3Device().UnbindShader<DomainShader>();
                break;
            default:
                assert(nullptr);
        }
    }

    void RenderPipeline::PrecompileShaders()
    {
        for (const auto& file : std::filesystem::directory_iterator("./"))
        {
            if (file.path().extension() == ".hlsl")
            {
                const auto prefix                     = file.path().filename().string();
                const auto filename_without_extension =
                        prefix.substr(0, prefix.find_last_of("."));

                if (prefix.starts_with("vs"))
                {
                    boost::shared_ptr<Graphics::Shader<ID3D11VertexShader>> shader =
                            boost::make_shared<Graphics::VertexShader>(
                                                                      filename_without_extension, file);

                    shader->Load();
                    GetResourceManager().AddResource(filename_without_extension, shader);
                }
                else if (prefix.starts_with("ps"))
                {
                    auto shader = boost::make_shared<Graphics::PixelShader>(
                                                                           filename_without_extension, file);

                    shader->Load();
                    GetResourceManager().AddResource(filename_without_extension, shader);
                }
                else if (prefix.starts_with("gs"))
                {
                    auto shader = boost::make_shared<Graphics::GeometryShader>(
                                                                              filename_without_extension, file);

                    shader->Load();
                    GetResourceManager().AddResource(filename_without_extension, shader);
                }
                else if (prefix.starts_with("cs"))
                {
                    auto shader = boost::make_shared<Graphics::ComputeShader>(
                                                                             filename_without_extension, file);

                    shader->Load();
                    GetResourceManager().AddResource(filename_without_extension, shader);
                }
                else if (prefix.starts_with("hs"))
                {
                    auto shader = boost::make_shared<Graphics::HullShader>(
                                                                          filename_without_extension, file);

                    shader->Load();
                    GetResourceManager().AddResource(filename_without_extension, shader);
                }
                else if (prefix.starts_with("ds"))
                {
                    auto shader = boost::make_shared<Graphics::DomainShader>(
                                                                            filename_without_extension, file);

                    shader->Load();
                    GetResourceManager().AddResource(filename_without_extension, shader);
                }
            }
        }
    }

    void RenderPipeline::InitializeSamplers()
    {
        const auto sampler = GetToolkitAPI().GetCommonStates()->LinearWrap();

        m_sampler_state_[SAMPLER_TEXTURE] = sampler;
        GetD3Device().BindSampler(
                                  m_sampler_state_[SAMPLER_TEXTURE], SHADER_PIXEL,
                                  SAMPLER_TEXTURE);
    }

    void RenderPipeline::PreUpdate(const float& dt)
    {
        // ** overriding DirectXTK common state
        GetD3Device().GetContext()->RSSetState(m_rasterizer_state_.Get());
        GetD3Device().GetContext()->OMSetBlendState(
                                                    m_blend_state_.Get(), nullptr,
                                                    0xFFFFFFFF);
        GetD3Device().GetContext()->OMSetDepthStencilState(
                                                           m_depth_stencil_state_.Get(), 1);
    }

    void RenderPipeline::PreRender(const float& dt) {}

    void RenderPipeline::Update(const float& dt) {}

    void RenderPipeline::Render(const float& dt) {}

    void RenderPipeline::FixedUpdate(const float& dt) {}

    void RenderPipeline::PostRender(const float& dt) {}

    void RenderPipeline::PostUpdate(const float& dt) {}

    void RenderPipeline::DrawIndexed(UINT index_count)
    {
        GetD3Device().GetContext()->DrawIndexed(index_count, 0, 0);
    }

    void RenderPipeline::TargetDepthOnly(ID3D11DepthStencilView* view, ID3D11DepthStencilState* state)
    {
        ID3D11RenderTargetView* pnullView = nullptr;
        GetD3Device().GetContext()->OMSetRenderTargets(1, &pnullView, view);
        GetD3Device().GetContext()->OMSetDepthStencilState(state, 0);
    }

    void RenderPipeline::SetViewport(const D3D11_VIEWPORT& viewport)
    {
        GetD3Device().GetContext()->RSSetViewports(1, &viewport);
    }

    void RenderPipeline::BindResources(
        UINT            slot,
        eShaderType     shader_type, ID3D11ShaderResourceView** textures, UINT size)
    {
        g_shader_rs_bind_map.at(shader_type)(
                                             GetD3Device().GetContext(), textures,
                                             slot, size);
    }

    void RenderPipeline::BindSampler(ID3D11SamplerState* sampler)
    {
        GetD3Device().BindSampler(sampler, SHADER_PIXEL, SAMPLER_SHADOW);
    }

    void RenderPipeline::ResetShaders()
    {
        GetD3Device().GetContext()->VSSetShader(nullptr, nullptr, 0);
        GetD3Device().GetContext()->PSSetShader(nullptr, nullptr, 0);
        GetD3Device().GetContext()->GSSetShader(nullptr, nullptr, 0);
        GetD3Device().GetContext()->CSSetShader(nullptr, nullptr, 0);
        GetD3Device().GetContext()->HSSetShader(nullptr, nullptr, 0);
        GetD3Device().GetContext()->DSSetShader(nullptr, nullptr, 0);
    }

    void RenderPipeline::DefaultDepthStencilState()
    {
        GetD3Device().GetContext()->OMSetDepthStencilState(
                                                           m_depth_stencil_state_.Get(), 0);
    }

    void RenderPipeline::SetShadowVP(const CBs::ShadowVPChunkCB& vp_chunk)
    {
        m_shadow_buffer_chunk_data_.SetData(
                                            GetD3Device().GetContext(),
                                            vp_chunk);
        GetD3Device().BindConstantBuffer(
                                         m_shadow_buffer_chunk_data_,
                                         CB_TYPE_SHADOW_CHUNK, SHADER_PIXEL);
    }

    void RenderPipeline::SetMaterial(const CBs::MaterialCB& material_buffer)
    {
        m_material_buffer_data_.SetData(
										GetD3Device().GetContext(),
										material_buffer);
        BindConstantBuffer(m_material_buffer_data_, SHADER_VERTEX);
        BindConstantBuffer(m_material_buffer_data_, SHADER_PIXEL);
    }

    void RenderPipeline::UnbindResource(UINT slot, eShaderType type)
    {
        ComPtr<ID3D11ShaderResourceView> null_view = nullptr;
        g_shader_rs_bind_map.at(type)(
                                      GetD3Device().GetContext(), null_view.GetAddressOf(),
                                      slot, 1);
    }
} // namespace Engine::Manager::Graphics
