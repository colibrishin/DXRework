#include "pch.h"
#include "egShader.hpp"
#include "egD3Device.hpp"
#include "egManagerHelper.hpp"
#include "egRenderPipeline.h"

SERIALIZER_ACCESS_IMPL
(
    Engine::Resources::Shader, 
    _ARTAG(_BSTSUPER(Resource))
    _ARTAG(m_domain_) _ARTAG(m_depth_flag_) _ARTAG(m_depth_test_) _ARTAG(m_depth_func_)
    _ARTAG(m_smp_filter_) _ARTAG(m_smp_address_) _ARTAG(m_smp_func_)
    _ARTAG(m_cull_mode_) _ARTAG(m_fill_mode_) _ARTAG(m_topology_)
)

namespace Engine::Resources
{
  void Shader::Load_INTERNAL()
  {
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error;
    UINT             flag = 0;

#if defined(_DEBUG)
    flag |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#endif

    for (const auto& [t, ep, v] : s_main_version)
    {
      // Try search for every type of shader.
      const auto res = D3DCompileFromFile
        (
         GetPath().c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
         ep.c_str(), v.c_str(), flag, 0,
         &blob, &error
        );

      // Print the warnings if there were.
      if (error)
      {
        const std::string error_message =
          static_cast<char*>(error->GetBufferPointer());

        // Silencing the no entry point error.
        if (error_message.find("X3501") == std::string::npos)
        {
          OutputDebugStringA(error_message.c_str());
        }
      }

      // If compiled, set shader.
      if (res == S_OK)
      {
        if (t == SHADER_VERTEX)
        {
          const auto ids = GetD3Device().GenerateInputDescription(blob.Get());
          GetD3Device().GetDevice()->CreateInputLayout
            (
             ids.data(), static_cast<UINT>(ids.size()), blob->GetBufferPointer(),
             blob->GetBufferSize(), m_il_.ReleaseAndGetAddressOf()
            );
          GetD3Device().GetDevice()->CreateVertexShader
            (
             blob->GetBufferPointer(), blob->GetBufferSize(),
             nullptr, m_vs_.ReleaseAndGetAddressOf()
            );
        }
        else if (t == SHADER_PIXEL)
        {
          GetD3Device().GetDevice()->CreatePixelShader
            (
             blob->GetBufferPointer(), blob->GetBufferSize(),
             nullptr, m_ps_.ReleaseAndGetAddressOf()
            );
        }
        else if (t == SHADER_GEOMETRY)
        {
          GetD3Device().GetDevice()->CreateGeometryShader
            (
             blob->GetBufferPointer(), blob->GetBufferSize(),
             nullptr, m_gs_.ReleaseAndGetAddressOf()
            );
        }
        else if (t == SHADER_HULL)
        {
          GetD3Device().GetDevice()->CreateHullShader
            (
             blob->GetBufferPointer(), blob->GetBufferSize(),
             nullptr, m_hs_.ReleaseAndGetAddressOf()
            );
        }
        else if (t == SHADER_DOMAIN)
        {
          GetD3Device().GetDevice()->CreateDomainShader
            (
             blob->GetBufferPointer(), blob->GetBufferSize(),
             nullptr, m_ds_.ReleaseAndGetAddressOf()
            );
        }
      }
    }

    if (!m_vs_) { throw std::runtime_error("Vertex shader is not found"); }

    D3D11_DEPTH_STENCIL_DESC dsd;
    dsd.DepthEnable                  = m_depth_flag_;
    dsd.DepthWriteMask               = m_depth_test_;
    dsd.DepthFunc                    = m_depth_func_;
    dsd.StencilEnable                = true;
    dsd.StencilReadMask              = 0xFF;
    dsd.StencilWriteMask             = 0xFF;
    dsd.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
    dsd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    dsd.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
    dsd.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;
    dsd.BackFace.StencilFailOp       = D3D11_STENCIL_OP_KEEP;
    dsd.BackFace.StencilDepthFailOp  = D3D11_STENCIL_OP_DECR;
    dsd.BackFace.StencilPassOp       = D3D11_STENCIL_OP_KEEP;
    dsd.BackFace.StencilFunc         = D3D11_COMPARISON_ALWAYS;

    GetD3Device().CreateDepthStencilState(dsd, m_dss_.ReleaseAndGetAddressOf());

    D3D11_RASTERIZER_DESC rd;
    rd.AntialiasedLineEnable = true;
    rd.DepthBias             = 0;
    rd.DepthBiasClamp        = 0.0f;
    rd.DepthClipEnable       = true;
    rd.CullMode              = m_cull_mode_;
    rd.FillMode              = m_fill_mode_;
    rd.FrontCounterClockwise = false;
    rd.MultisampleEnable     = false;
    rd.ScissorEnable         = false;
    rd.SlopeScaledDepthBias  = 0.0f;

    GetD3Device().CreateRasterizerState(rd, m_rs_.ReleaseAndGetAddressOf());

    D3D11_SAMPLER_DESC sd;
    sd.Filter         = m_smp_filter_;
    sd.AddressU       = m_smp_address_;
    sd.AddressV       = m_smp_address_;
    sd.AddressW       = m_smp_address_;
    sd.MipLODBias     = 0.0f;
    sd.MaxAnisotropy  = 1;
    sd.ComparisonFunc = m_smp_func_;

    GetD3Device().CreateSampler(sd, m_ss_.ReleaseAndGetAddressOf());
  }

  Shader::Shader(
    const EntityName& name, const std::filesystem::path& path, const eShaderDomain      domain,
    const UINT        depth, const UINT                  rasterizer, const D3D11_FILTER filter, const UINT sampler
  )
    : Resource(path, RES_T_SHADER),
      m_domain_(domain),
      m_depth_flag_(depth != 0),
      m_depth_test_(static_cast<D3D11_DEPTH_WRITE_MASK>(depth & 1)),
      m_depth_func_(static_cast<D3D11_COMPARISON_FUNC>(std::log2(depth >> 1) + 1)),
      m_smp_filter_(filter),
      m_smp_address_(static_cast<D3D11_TEXTURE_ADDRESS_MODE>((sampler & shader_sampler_address_mask) + 1)),
      m_smp_func_(static_cast<D3D11_COMPARISON_FUNC>(std::log2(sampler >> 3))),
      m_cull_mode_(static_cast<D3D11_CULL_MODE>((rasterizer & 2) + 1)),
      m_fill_mode_(static_cast<D3D11_FILL_MODE>((rasterizer >> 2) + 1)),
      m_topology_(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
  {
    SetName(name);
  }

  void Shader::Initialize() {}

  void Shader::PreUpdate(const float& dt) {}

  void Shader::Update(const float& dt) {}

  void Shader::FixedUpdate(const float& dt) {}

  void Shader::PostUpdate(const float& dt) {}

  void Shader::PreRender(const float& dt) {}

  void Shader::Unload_INTERNAL()
  {
    m_vs_.Reset();
    m_ps_.Reset();
    m_gs_.Reset();
    m_hs_.Reset();
    m_ds_.Reset();
    m_il_.Reset();
    m_ss_.Reset();
    m_dss_.Reset();
    m_rs_.Reset();
  }

  void Shader::OnDeserialized() {}

  void Shader::Render(const float& dt)
  {
    GetD3Device().GetContext()->IASetInputLayout(m_il_.Get());
    GetD3Device().GetContext()->IASetPrimitiveTopology(m_topology_);
    GetD3Device().GetContext()->VSSetShader(m_vs_.Get(), nullptr, 0);
    GetD3Device().GetContext()->PSSetShader(m_ps_.Get(), nullptr, 0);
    GetD3Device().GetContext()->GSSetShader(m_gs_.Get(), nullptr, 0);
    GetD3Device().GetContext()->HSSetShader(m_hs_.Get(), nullptr, 0);
    GetD3Device().GetContext()->DSSetShader(m_ds_.Get(), nullptr, 0);

    GetRenderPipeline().SetDepthStencilState(m_dss_.Get());
    GetRenderPipeline().SetRasterizerState(m_rs_.Get());
    GetRenderPipeline().SetSamplerState(m_ss_.Get());
  }

  void Shader::PostRender(const float& dt)
  {
    GetD3Device().GetContext()->IASetInputLayout(nullptr);
    GetD3Device().GetContext()->VSSetShader(nullptr, nullptr, 0);
    GetD3Device().GetContext()->PSSetShader(nullptr, nullptr, 0);
    GetD3Device().GetContext()->GSSetShader(nullptr, nullptr, 0);
    GetD3Device().GetContext()->CSSetShader(nullptr, nullptr, 0);
    GetD3Device().GetContext()->HSSetShader(nullptr, nullptr, 0);
    GetD3Device().GetContext()->DSSetShader(nullptr, nullptr, 0);

    GetRenderPipeline().DefaultDepthStencilState();
    GetRenderPipeline().DefaultRasterizerState();
    GetRenderPipeline().DefaultSamplerState();
  }

  void Shader::SetTopology(D3D11_PRIMITIVE_TOPOLOGY topology) { m_topology_ = topology; }

  eShaderDomain Shader::GetDomain() const { return m_domain_; }

  boost::weak_ptr<Shader> Shader::Get(const std::string& name)
  {
    return Manager::ResourceManager::GetInstance().GetResource<Shader>(name);
  }

  boost::shared_ptr<Shader> Shader::Create(
    const std::string& name, const std::filesystem::path& path, const eShaderDomain domain, const UINT depth,
    const UINT         rasterizer, const D3D11_FILTER     filter, const UINT        sampler
  )
  {
    if (const auto              pcheck = GetResourceManager().GetResourceByRawPath<Shader>
      (path).lock(); const auto ncheck = GetResourceManager().GetResource<Shader>(name).lock()) { return ncheck; }

    const auto obj = boost::make_shared<Shader>(name, path, domain, depth, rasterizer, filter, sampler);
    GetResourceManager().AddResource(name, obj);
    return obj;
  }

  void Shader::OnSerialized() {}

  Shader::Shader()
    : Resource("", RES_T_SHADER),
      m_domain_(),
      m_depth_flag_(false),
      m_depth_test_(),
      m_depth_func_(),
      m_smp_filter_(),
      m_smp_address_(),
      m_smp_func_(),
      m_cull_mode_(),
      m_fill_mode_(),
      m_topology_(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST) { }
} // namespace Engine::Graphic
