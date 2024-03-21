#include "pch.h"
#include "egComputeShader.h"

#include "egParticleRenderer.h"
#include "egTexture.h"

SERIALIZE_IMPL
(
 Engine::Resources::ComputeShader,
 _ARTAG(_BSTSUPER(Shader))
 _ARTAG(m_group_)
 _ARTAG(m_thread_)
)

namespace Engine::Resources
{
  void ComputeShader::Dispatch()
  {
    preDispatch();

    if (std::accumulate(m_group_, m_group_ + 3, 0) == 0)
    {
      GetDebugger().Log("ComputeShader::Dispatch() : Group is not set. Ignore dispatching...");
      return;
    }

    if (std::accumulate(m_group_, m_group_ + 3, 1, std::multiplies()) > 1 << 16)
    {
      GetDebugger().Log("ComputeShader::Dispatch() : Group is too large. Ignore dispatching...");
      return;
    }

    if (std::accumulate(m_thread_, m_thread_ + 3, 0) == 0)
    {
      GetDebugger().Log("ComputeShader::Dispatch() : Thread is not set. Ignore dispatching...");
      return;
    }

    if (std::accumulate(m_thread_, m_thread_ + 3, 1, std::multiplies()) > 1024)
    {
      GetDebugger().Log("ComputeShader::Dispatch() : Thread is too large. Ignore dispatching...");
      return;
    }

    GetD3Device().GetContext()->CSSetShader(m_cs_.Get(), nullptr, 0);
    GetD3Device().GetContext()->Dispatch(m_group_[0], m_group_[1], m_group_[2]);

    postDispatch();

    GetD3Device().GetContext()->CSSetShader(nullptr, nullptr, 0);

    std::fill_n(m_group_, 3, 1);
  }

  ComputeShader::ComputeShader(
    const std::string&           name,
    const std::filesystem::path& path,
    const std::array<UINT, 3>&   thread
  )
    : Shader
    (
     name, path, SHADER_DOMAIN_OPAQUE, 0, SHADER_RASTERIZER_CULL_NONE,
     D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT, SHADER_SAMPLER_NEVER
    )
  {
    SetPath(path);
    std::ranges::copy(thread, m_thread_);
  }

  ParamBase& ComputeShader::getParam(const StrongParticleRenderer& pr)
  {
    return pr->m_params_;
  }

  InstanceParticles& ComputeShader::getInstances(const StrongParticleRenderer& pr)
  {
    return pr->m_instances_;
  }

  void ComputeShader::SetGroup(const std::array<UINT, 3>& group) { std::ranges::copy(group, m_group_); }

  std::array<UINT, 3> ComputeShader::GetThread() const
  {
    std::array<UINT, 3> thread;
    thread[0] = m_thread_[0];
    thread[1] = m_thread_[1];
    thread[2] = m_thread_[2];
    return thread;
  }

  void ComputeShader::PostRender(const float& dt) {}

  void ComputeShader::PostUpdate(const float& dt) {}

  void ComputeShader::PreRender(const float& dt) {}

  void ComputeShader::PreUpdate(const float& dt) {}

  void ComputeShader::Render(const float& dt) {}

  void ComputeShader::FixedUpdate(const float& dt) {}

  void ComputeShader::Update(const float& dt) {}

  void ComputeShader::Initialize()
  {
    Shader::Initialize();
    std::fill_n(m_group_, 3, 1);
  }

  void ComputeShader::Load_INTERNAL()
  {
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error;
    UINT             flag = 0;

    if constexpr (g_debug)
    {
      flag |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
    }

    const auto [type, entry, version] = s_main_version[SHADER_COMPUTE];

    const auto res = D3DCompileFromFile
      (
       GetPath().c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
       entry.c_str(), version.c_str(), flag, 0,
       &blob, &error
      );

    if (error)
    {
      const std::string error_message =
        static_cast<char*>(error->GetBufferPointer());
      OutputDebugStringA(error_message.c_str());
    }

    if (FAILED(res))
    {
      throw std::exception("ComputeShader::Load_INTERNAL() : Failed to compile shader.");
    }

    GetD3Device().GetDevice()->CreateComputeShader
      (
       blob->GetBufferPointer(), blob->GetBufferSize(),
       nullptr, m_cs_.ReleaseAndGetAddressOf()
      );

    loadDerived();
  }

  void ComputeShader::Unload_INTERNAL()
  {
    m_cs_.Reset();
    unloadDerived();
  }

  ComputeShader::ComputeShader()
    : Shader
      (
       "", "", SHADER_DOMAIN_OPAQUE, 0, SHADER_RASTERIZER_CULL_NONE,
       D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT, SHADER_SAMPLER_NEVER
      ),
      m_thread_{1,},
      m_group_{1,} {}
}
