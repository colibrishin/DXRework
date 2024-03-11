#pragma once
#include <filesystem>
#include "egCommon.hpp"
#include "egEntity.hpp"
#include "egResource.h"

namespace Engine::Resources
{
  class Shader : public Abstract::Resource
  {
  public:
    RESOURCE_T(RES_T_SHADER)

    Shader(
      const EntityName&  name, const std::filesystem::path& path,
      eShaderDomain      domain, eShaderDepths              depth,
      eShaderRasterizers rasterizer, D3D11_FILTER           sampler_filter, eShaderSamplers sampler,
      D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    );
    ~Shader() override = default;

    void Initialize() override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PreRender(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostUpdate(const float& dt) override;

    void Render(const float& dt) override;
    void PostRender(const float& dt) override;

    void SetTopology(D3D11_PRIMITIVE_TOPOLOGY topology);

    eShaderDomain GetDomain() const;

    static boost::weak_ptr<Shader>   Get(const std::string& name);
    static boost::shared_ptr<Shader> Create(
      const std::string &           name,
      const std::filesystem::path & path,
      const eShaderDomain           domain,
      const UINT                    depth,
      const UINT                    rasterizer,
      const D3D11_FILTER            filter,
      const UINT                    sampler, D3D11_PRIMITIVE_TOPOLOGY topology
    );

  protected:
    void OnSerialized() override;
    void OnDeserialized() override;

    void Load_INTERNAL() override;
    void Unload_INTERNAL() override;
    
    inline static std::vector<std::tuple<eShaderType, std::string, std::string>> s_main_version =
    {
      {SHADER_VERTEX, "vs_main", "vs_5_0"},
      {SHADER_PIXEL, "ps_main", "ps_5_0"},
      {SHADER_GEOMETRY, "gs_main", "gs_5_0"},
      {SHADER_COMPUTE, "cs_main", "cs_5_0"},
      {SHADER_HULL, "hs_main", "hs_5_0"},
      {SHADER_DOMAIN, "ds_main", "ds_5_0"}
    };

  private:
    Shader();
    SERIALIZER_ACCESS

    eShaderDomain              m_domain_;
    bool                       m_depth_flag_;
    D3D11_DEPTH_WRITE_MASK     m_depth_test_;
    D3D11_COMPARISON_FUNC      m_depth_func_;
    D3D11_FILTER               m_smp_filter_;
    D3D11_TEXTURE_ADDRESS_MODE m_smp_address_;
    D3D11_COMPARISON_FUNC      m_smp_func_;
    D3D11_CULL_MODE            m_cull_mode_;
    D3D11_FILL_MODE            m_fill_mode_;

    D3D11_PRIMITIVE_TOPOLOGY  m_topology_;
    ComPtr<ID3D11InputLayout> m_il_;

    ComPtr<ID3D11VertexShader>   m_vs_;
    ComPtr<ID3D11PixelShader>    m_ps_;
    ComPtr<ID3D11GeometryShader> m_gs_;
    ComPtr<ID3D11HullShader>     m_hs_;
    ComPtr<ID3D11DomainShader>   m_ds_;

    ComPtr<ID3D11DepthStencilState> m_dss_;
    ComPtr<ID3D11RasterizerState>   m_rs_;
    ComPtr<ID3D11SamplerState>      m_ss_;
  };
} // namespace Engine::Graphic

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Shader);
