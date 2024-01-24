#pragma once
#include "egMacro.h"

namespace Engine::Resources
{
  class ComputeShader : public Shader
  {
  public:
    void SetTexture(const WeakTexture& tex);
    void Dispatch(const std::array<UINT, 3>& group);

	protected:
    ComputeShader(const std::filesystem::path& path, const std::array<UINT, 3>& thread)
      : m_thread_(thread) { SetPath(path); }

    virtual void preDispatch() = 0;
    virtual void postDispatch() = 0;

  private:
    void PostRender(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void PreUpdate(const float& dt) override;
    void Render(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void Initialize() override;

    void Load_INTERNAL() override;
    void Unload_INTERNAL() override;

  private:
    WeakTexture m_tex_;

    ComPtr<ID3D11ComputeShader> m_cs_;
    std::array<UINT, 3> m_thread_;

  };
}
