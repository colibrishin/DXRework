#include "pch.h"
#include "egRenderPipeline.h"

#include <filesystem>

#include "egD3Device.hpp"
#include "egManagerHelper.hpp"
#include "egShader.hpp"
#include "egToolkitAPI.h"
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

  void RenderPipeline::SetTopology(const D3D11_PRIMITIVE_TOPOLOGY& topology)
  {
    GetD3Device().GetContext()->IASetPrimitiveTopology(topology);
  }

  void RenderPipeline::SetDepthStencilState(ID3D11DepthStencilState* state)
  {
    GetD3Device().GetContext()->OMSetDepthStencilState(state, 0);
  }

  void RenderPipeline::SetRasterizerState(ID3D11RasterizerState* state)
  {
    GetD3Device().GetContext()->RSSetState(state);
  }

  void RenderPipeline::SetSamplerState(ID3D11SamplerState* sampler)
  {
    GetD3Device().BindSampler(sampler, SHADER_PIXEL, SAMPLER_TEXTURE);
  }

  void RenderPipeline::DefaultRenderTarget() const
  {
    GetD3Device().GetContext()->OMSetDepthStencilState
      (
       m_depth_stencil_state_.Get(), 0
      );
    GetD3Device().UpdateRenderTarget();
  }

  void RenderPipeline::DefaultViewport() const { GetD3Device().UpdateViewport(); }

  void RenderPipeline::SetWireframeState() const
  {
    GetD3Device().GetContext()->RSSetState
      (
       GetToolkitAPI().GetCommonStates()->Wireframe()
      );
  }

  void RenderPipeline::SetFillState() const { GetD3Device().GetContext()->RSSetState(m_rasterizer_state_.Get()); }

  void RenderPipeline::SetNoneCullState() const
  {
    GetD3Device().GetContext()->RSSetState
      (
       GetToolkitAPI().GetCommonStates()->CullNone()
      );
  }

  void RenderPipeline::SetFrontCullState() const
  {
    GetD3Device().GetContext()->RSSetState
      (
       GetToolkitAPI().GetCommonStates()->CullClockwise()
      );
  }

  void RenderPipeline::BindVertexBuffer(ID3D11Buffer* buffer)
  {
    constexpr UINT stride = sizeof(VertexElement);
    constexpr UINT offset = 0;
    GetD3Device().GetContext()->IASetVertexBuffers
      (
       0, 1, &buffer, &stride,
       &offset
      );
  }

  void RenderPipeline::BindIndexBuffer(ID3D11Buffer* buffer)
  {
    GetD3Device().GetContext()->IASetIndexBuffer(buffer, DXGI_FORMAT_R32_UINT, 0);
  }

  void RenderPipeline::UnbindVertexBuffer()
  {
    constexpr UINT stride      = 0;
    constexpr UINT offset      = 0;
    ID3D11Buffer*  null_buffer = nullptr;
    GetD3Device().GetContext()->IASetVertexBuffers
      (
       0, 1, &null_buffer, &stride,
       &offset
      );
  }

  void RenderPipeline::UnbindIndexBuffer()
  {
    ID3D11Buffer* null_buffer = nullptr;
    GetD3Device().GetContext()->IASetIndexBuffer(null_buffer, DXGI_FORMAT_R32_UINT, 0);
  }

  void RenderPipeline::BindResource(
    UINT                       slot,
    eShaderType                shader_type,
    ID3D11ShaderResourceView** texture
  ) { g_shader_rs_bind_map.at(shader_type)(GetD3Device().GetContext(), texture, slot, 1); }

  RenderPipeline::~RenderPipeline() { ResetShaders(); }

  void RenderPipeline::Initialize()
  {
    GetD3Device().CreateConstantBuffer(m_wvp_buffer_data_);
    GetD3Device().CreateConstantBuffer(m_transform_buffer_data_);
    GetD3Device().CreateConstantBuffer(m_material_buffer_data_);
    GetD3Device().CreateConstantBuffer(m_param_buffer_data_);

    PrecompileShaders();
    InitializeSamplers();

    GetD3Device().CreateBlendState(m_blend_state_.GetAddressOf());
    GetD3Device().CreateRasterizer
      (
       m_rasterizer_state_.GetAddressOf(),
       D3D11_FILL_SOLID
      );
    GetD3Device().CreateDepthStencilState(m_depth_stencil_state_.GetAddressOf());
  }

  void RenderPipeline::PrecompileShaders()
  {
    Shader::Create
      (
       "default", "./default.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D11_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS
      );

    Shader::Create
      (
       "color", "./color.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D11_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS
      );

    Shader::Create
      (
       "skybox", "./skybox.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_NONE | SHADER_RASTERIZER_FILL_SOLID,
       D3D11_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS
      );

    Shader::Create
      (
       "specular_normal", "./specular_normal.hlsl",
       SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D11_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS
      );

    Shader::Create
      (
       "normal", "./normal.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D11_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS
      );

    Shader::Create
      (
       "refraction", "./refraction.hlsl", SHADER_DOMAIN_POST_PROCESS,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D11_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS
      );

    Shader::Create
      (
       "specular_tex", "./specular_tex.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D11_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS
      );

    Shader::Create
      (
       "specular", "./specular.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D11_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS
      );

    Shader::Create
      (
       "cascade_shadow_stage1", "./cascade_shadow_stage1.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D11_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_CLAMP | SHADER_SAMPLER_ALWAYS
      );
  }

  void RenderPipeline::InitializeSamplers()
  {
    const auto sampler = GetToolkitAPI().GetCommonStates()->LinearWrap();

    m_sampler_state_[SAMPLER_TEXTURE] = sampler;
    GetD3Device().BindSampler
      (
       m_sampler_state_[SAMPLER_TEXTURE], SHADER_PIXEL,
       SAMPLER_TEXTURE
      );
  }

  void RenderPipeline::PreUpdate(const float& dt)
  {
    // ** overriding DirectXTK common state
    GetD3Device().GetContext()->RSSetState(m_rasterizer_state_.Get());
    GetD3Device().GetContext()->OMSetBlendState
      (
       m_blend_state_.Get(), nullptr,
       0xFFFFFFFF
      );
    GetD3Device().GetContext()->OMSetDepthStencilState
      (
       m_depth_stencil_state_.Get(), 1
      );
  }

  void RenderPipeline::PreRender(const float& dt) {}

  void RenderPipeline::Update(const float& dt) {}

  void RenderPipeline::Render(const float& dt) {}

  void RenderPipeline::FixedUpdate(const float& dt) {}

  void RenderPipeline::PostRender(const float& dt) {}

  void RenderPipeline::PostUpdate(const float& dt) {}

  void RenderPipeline::DrawIndexed(UINT index_count) { GetD3Device().GetContext()->DrawIndexed(index_count, 0, 0); }

  void RenderPipeline::DrawIndexedInstanced(UINT index_count, UINT instance_count)
  {
    GetD3Device().GetContext()->DrawIndexedInstanced(index_count, instance_count, 0, 0, 0);
  }

  void RenderPipeline::TargetDepthOnly(ID3D11DepthStencilView* view)
  {
    ID3D11RenderTargetView* pnullView = nullptr;
    GetD3Device().GetContext()->OMSetRenderTargets(1, &pnullView, view);
  }

  void RenderPipeline::SetViewport(const D3D11_VIEWPORT& viewport)
  {
    GetD3Device().GetContext()->RSSetViewports(1, &viewport);
  }

  void RenderPipeline::BindResources(
    UINT        slot,
    eShaderType shader_type, ID3D11ShaderResourceView** textures, UINT size
  )
  {
    g_shader_rs_bind_map.at(shader_type)
      (
       GetD3Device().GetContext(), textures,
       slot, size
      );
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

  void RenderPipeline::DefaultDepthStencilState() const
  {
    GetD3Device().GetContext()->OMSetDepthStencilState
      (
       m_depth_stencil_state_.Get(), 0
      );
  }

  void RenderPipeline::DefaultRasterizerState() const { GetD3Device().GetContext()->RSSetState(m_rasterizer_state_.Get()); }

  void RenderPipeline::DefaultSamplerState() const
  {
    GetD3Device().BindSampler
      (
       m_sampler_state_.at(SAMPLER_TEXTURE), SHADER_PIXEL,
       SAMPLER_TEXTURE
      );
  }

  void RenderPipeline::SetMaterial(const CBs::MaterialCB& material_buffer)
  {
    m_material_buffer_data_.SetData
      (
       GetD3Device().GetContext(),
       material_buffer
      );
    BindConstantBuffer(m_material_buffer_data_, SHADER_VERTEX);
    BindConstantBuffer(m_material_buffer_data_, SHADER_PIXEL);
  }

  void RenderPipeline::UnbindResource(UINT slot, eShaderType type)
  {
    ComPtr<ID3D11ShaderResourceView> null_view = nullptr;
    g_shader_rs_bind_map.at(type)
      (
       GetD3Device().GetContext(), null_view.GetAddressOf(),
       slot, 1
      );
  }
} // namespace Engine::Manager::Graphics
