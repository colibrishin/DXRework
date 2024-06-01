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
           .Alignment = 0,
           .Width = g_window_width,
           .Height = g_window_height,
           .DepthOrArraySize = 0,
           .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
           .Flags = D3D12_RESOURCE_FLAG_NONE,
           .MipsLevel = 1,
           .Layout = D3D12_TEXTURE_LAYOUT_64KB_STANDARD_SWIZZLE,
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

    void RenderFinished(const CommandPair & cmd) const;

    void BindReflectionMap(const CommandPair & cmd, const DescriptorPtr & heap) const;
    void UnbindReflectionMap(const CommandPair& cmd) const;

  private:
    friend struct SingletonDeleter;
    ~ReflectionEvaluator() override = default;

    Resources::Texture2D m_copy_; 
  };
}
