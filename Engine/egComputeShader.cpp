#include "pch.h"
#include "egComputeShader.h"

#include "egTexture.h"

namespace Engine::Resources
{
  void ComputeShader::SetTexture(const WeakTexture& tex)
  {
    if (const auto t = tex.lock()) { m_tex_ = tex; }
  }

  void ComputeShader::Dispatch(const std::array<UINT, 3>& group)
  {
    if (m_tex_.expired())
    {
      GetDebugger().Log("ComputeShader::Dispatch() : Texture is not set. Ignore dispatching...");
      return;
    }

    preDispatch();

    const auto tex = m_tex_.lock();
    tex->BindAs(D3D11_BIND_UNORDERED_ACCESS, BIND_SLOT_TEX, 0, SHADER_COMPUTE);
    tex->Render(0.f);

    GetD3Device().GetContext()->CSSetShader(m_cs_.Get(), nullptr, 0);
    GetD3Device().GetContext()->Dispatch(group[0], group[1], group[2]);
    GetD3Device().GetContext()->CSSetShader(nullptr, nullptr, 0);

    tex->PostRender(0.f);

    postDispatch();
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
