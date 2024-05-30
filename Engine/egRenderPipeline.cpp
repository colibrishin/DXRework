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
    m_transform_buffer_data_.SetData(&matrix);
  }

  void RenderPipeline::SetPerspectiveMatrix(const CBs::PerspectiveCB& matrix)
  {
    m_wvp_buffer_data_.SetData(&matrix);
  }

  RenderPipeline::TempParamTicket&& RenderPipeline::SetParam(const ParamBase& param)
  {
    TempParamTicket ticket(m_param_buffer_); // RAII

    m_param_buffer_ = static_cast<CBs::ParamCB>(param);
    m_param_buffer_data_.SetData(&m_param_buffer_);
    
    return std::move(ticket);
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

  void RenderPipeline::BindVertexBuffer(const D3D12_VERTEX_BUFFER_VIEW& view)
  {
    GetD3Device().GetCommandList()->IASetVertexBuffers(0, 1, &view);
  }

  void RenderPipeline::InitializeStaticBuffers()
  {
    m_transform_buffer_data_.Create(nullptr);
    m_wvp_buffer_data_.Create(nullptr);
    m_material_buffer_data_.Create(nullptr);
    m_param_buffer_data_.Create(nullptr);
  }
  void RenderPipeline::BindResource(
    UINT                       slot,
    eShaderType                shader_type,
    ID3D11ShaderResourceView** texture
  ) { g_shader_rs_bind_map.at(shader_type)(GetD3Device().GetContext(), texture, slot, 1); }

  RenderPipeline::~RenderPipeline() { ResetShaders(); }

  void RenderPipeline::Initialize()
  {
    InitializeStaticBuffers();
    PrecompileShaders();
    InitializeRootSignature();
    InitializeDefaultPSO();
  }

  void RenderPipeline::PrecompileShaders()
  {
    Shader::Create
      (
       "default", "./default.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );

    Shader::Create
      (
       "color", "./color.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );

    Shader::Create
      (
       "skybox", "./skybox.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_NONE | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );

    Shader::Create
      (
       "specular_normal", "./specular_normal.hlsl",
       SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );

    Shader::Create
      (
       "normal", "./normal.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );

    Shader::Create
      (
       "refraction", "./refraction.hlsl", SHADER_DOMAIN_POST_PROCESS,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );

    Shader::Create
      (
       "specular_tex", "./specular_tex.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );

    Shader::Create
      (
       "specular", "./specular.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );

    Shader::Create
      (
       "cascade_shadow_stage1", "./cascade_shadow_stage1.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_CLAMP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );

    const auto billboard = Shader::Create
      (
       "billboard", "./billboard.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_NONE | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_POINTLIST
      );

    Shader::Create
      (
       "intensity_test", "./intensity_test.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_POINT,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_NEVER, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );

    Shader::Create
      (
       "atlas", "./atlas.hlsl", SHADER_DOMAIN_OPAQUE,
       SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
       SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
       D3D12_FILTER_MIN_MAG_MIP_LINEAR,
       SHADER_SAMPLER_WRAP | SHADER_SAMPLER_ALWAYS, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
      );
  }

  void RenderPipeline::InitializeDefaultPSO()
  {
    
  }

  void RenderPipeline::InitializeRootSignature()
  {
    CD3DX12_DESCRIPTOR_RANGE1 ranges[4];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 2, 0);
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, BIND_SLOT_UAV_END, 0);
    ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, BIND_SLOT_END, 0);
    ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, CB_TYPE_END, 0);

    CD3DX12_ROOT_PARAMETER1 root_parameters[3];
    root_parameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
    root_parameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
    root_parameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);
    root_parameters[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_ALL);

    CD3DX12_STATIC_SAMPLER_DESC static_sampler_desc[2];
    static_sampler_desc[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
    static_sampler_desc[1].Init(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT);

    const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc
    (
      _countof(root_parameters),
      root_parameters,
      _countof(static_sampler_desc),
      static_sampler_desc
    );

    ComPtr<ID3DBlob> signature_blob = nullptr;
    DX::ThrowIfFailed
      (
       D3D12SerializeVersionedRootSignature
       (
        &root_signature_desc, signature_blob.GetAddressOf(), nullptr
       )
      );

    DX::ThrowIfFailed
    (
      GetD3Device().GetDevice()->CreateRootSignature
      (
        0,
        signature_blob->GetBufferPointer(),
        signature_blob->GetBufferSize(),
        IID_PPV_ARGS(m_root_signature_.ReleaseAndGetAddressOf())
        )
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

  void RenderPipeline::BindResources(
    UINT        slot,
    eShaderType shader_type, ID3D11UnorderedAccessView** textures, UINT size
  )
  {
    if (shader_type == SHADER_COMPUTE)
    {
      GetD3Device().GetContext()->CSSetUnorderedAccessViews
        (
         slot, size, textures, nullptr
        );
    }
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

  ID3D12RootSignature* RenderPipeline::GetRootSignature() const
  {
    return m_root_signature_.Get();
  }

  void RenderPipeline::SetPSO(const StrongShader& Shader)
  {
    const auto& shader_description = Shader->GetPipelineStateDesc();
    
    DX::ThrowIfFailed
      (
       GetD3Device().GetDevice()->CreateGraphicsPipelineState
       (
        &shader_description,
        IID_PPV_ARGS(m_pipeline_state_.ReleaseAndGetAddressOf())
       )
      );

    GetD3Device().GetCommandList()->SetPipelineState(m_pipeline_state_.Get());
  }

  void RenderPipeline::SetMaterial(const CBs::MaterialCB& material_buffer)
  {
    m_material_buffer_data_.SetData(&material_buffer);
  }
} // namespace Engine::Manager::Graphics
