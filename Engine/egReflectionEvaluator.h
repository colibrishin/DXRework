#pragma once
#include "pch.h"

#include "egGlobal.h"
#include "egTexture2D.h"

namespace Engine::Manager::Graphics
{
  class ReflectionEvaluator : public Abstract::Singleton<ReflectionEvaluator>
  {
  public:
    ReflectionEvaluator(SINGLETON_LOCK_TOKEN)
      : Singleton(),
        m_copy_
        (
         "", {
           .Width = g_window_width,
           .Height = g_window_height,
           .Depth = 0,
           .ArraySize = 1,
           .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
           .CPUAccessFlags = 0,
           .BindFlags = D3D11_BIND_SHADER_RESOURCE,
           .MipsLevel = 1,
           .MiscFlags = 0,
           .Usage = D3D11_USAGE_DEFAULT,
           .SampleDesc = { .Count = 1, .Quality = 0 }
         }
        ) {}

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void Initialize() override;

    void RenderFinished();

  private:
    friend struct SingletonDeleter;
    ~ReflectionEvaluator() override = default;

    Resources::Texture2D m_copy_; 
  };
}
