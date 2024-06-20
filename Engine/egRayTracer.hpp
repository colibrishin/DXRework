#pragma once
#include "egManager.hpp"
#include "egMaterial.h"
#include "egModelRenderer.h"
#include "egStructuredBuffer.hpp"

namespace Engine::Manager::Graphics
{
  class RayTracer : public Abstract::Singleton<RayTracer>
  {
  public:
    explicit RayTracer(SINGLETON_LOCK_TOKEN)
      : Singleton() {}

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void Initialize() override;

    bool   Ready() const;
    UINT64 GetInstanceCount() const;

    const Graphics::StructuredBuffer<Graphics::SBs::LightSB>& GetLightSB() const;

    void WaitForBuild() const
    {
      m_built_.wait(true);
    }

    void RenderPass(ID3D12GraphicsCommandList4* cmd, const std::function<bool(const StrongObjectBase&)>& predicate);

  private:
    friend struct SingletonDeleter;
    ~RayTracer() override = default;

    std::atomic<bool> m_built_;

    std::vector<SBs::LightSB> m_light_buffers_;
    StructuredBuffer<SBs::LightSB> m_light_buffer_data_;
    std::vector<Graphics::StructuredBuffer<Graphics::SBs::InstanceSB>> m_tmp_instances_;
  };
}
