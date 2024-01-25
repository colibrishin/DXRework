#include "pch.h"
#include "egComputeShader.h"

#include "egTexture.h"

namespace Engine::Resources
{
  void ComputeShader::Dispatch()
  {
    preDispatch();

    if (std::accumulate(m_group_.begin(), m_group_.end(), 0) == 0)
    {
      GetDebugger().Log("ComputeShader::Dispatch() : Group is not set. Ignore dispatching...");
      return;
    }

    if (std::accumulate(m_group_.begin(), m_group_.end(), 1, std::multiplies()) > 1024)
    {
      GetDebugger().Log("ComputeShader::Dispatch() : Group is too large. Ignore dispatching...");
      return;
    }

    if (std::accumulate(m_thread_.begin(), m_thread_.end(), 0) == 0)
    {
      GetDebugger().Log("ComputeShader::Dispatch() : Thread is not set. Ignore dispatching...");
      return;
    }

    if (std::accumulate(m_thread_.begin(), m_thread_.end(), 1, std::multiplies()) > 1024)
    {
      GetDebugger().Log("ComputeShader::Dispatch() : Thread is too large. Ignore dispatching...");
      return;
    }

    GetD3Device().GetContext()->CSSetShader(m_cs_.Get(), nullptr, 0);
    GetD3Device().GetContext()->Dispatch(m_group_[0], m_group_[1], m_group_[3]);

    postDispatch();

    GetD3Device().GetContext()->CSSetShader(nullptr, nullptr, 0);
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
  }

  void ComputeShader::Load_INTERNAL()
  {
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error;
    UINT             flag = 0;

#if defined(_DEBUG)
    flag |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#endif

    const auto [type, entry, version] = s_main_version[SHADER_COMPUTE];

    DX::ThrowIfFailed
      (
       D3DCompileFromFile
       (
        GetPath().c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entry.c_str(), version.c_str(), flag, 0,
        &blob, &error
       )
      );

    GetD3Device().GetDevice()->CreateComputeShader
      (
       blob->GetBufferPointer(), blob->GetBufferSize(),
       nullptr, m_cs_.ReleaseAndGetAddressOf()
      );
  }

  void ComputeShader::Unload_INTERNAL() { m_cs_.Reset(); }
}
