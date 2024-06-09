#pragma once
#include "egManager.hpp"
#include "egMaterial.h"
#include "egModelRenderer.h"
#include "egStructuredBuffer.hpp"

namespace Engine::Manager::Graphics
{
  class Renderer : public Abstract::Singleton<Renderer>
  {
  private:
    using CandidateTuple = std::tuple<WeakObjectBase, WeakMaterial, tbb::concurrent_vector<SBs::InstanceSB>>;
    using RenderMap = tbb::concurrent_hash_map<eRenderComponentType, tbb::concurrent_vector<CandidateTuple>>;

  public:
    explicit Renderer(SINGLETON_LOCK_TOKEN)
      : Singleton(),
        m_b_ready_(false) {}

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void Initialize() override;

    void AppendAdditionalStructuredBuffer(const Weak<StructuredBufferBase> & sb_ptr);

    bool Ready() const;
    void RenderPass(
      const float dt,
      eShaderDomain domain,
      bool shader_bypass,
      const CommandPair &cmd, const std::function<bool(const StrongObjectBase&)> &predicate, const std::function<void(
        const CommandPair&, const DescriptorPtr&)> &initial_setup, const std::function<void(const CommandPair&,
        const DescriptorPtr&)> &post_setup, const std::vector<Weak<StructuredBufferBase>> &
      additional_structured_buffers
    );

  private:
    friend struct SingletonDeleter;
    ~Renderer() override = default;

    void renderPassImpl(
      const float            dt,
      eShaderDomain          domain,
      bool                   shader_bypass,
      const StrongMaterial & material, const CommandPair
      &                      cmd, const DescriptorPtr & heap, const std::vector<SBs::InstanceSB> & structured_buffers
    );

    void preMappingModel(const StrongRenderComponent& rc);
    void preMappingParticle(const StrongRenderComponent& rc);

    bool m_b_ready_;

    std::vector<Weak<StructuredBufferBase>> m_additional_structured_buffers_;
    tbb::concurrent_vector<StructuredBuffer<SBs::InstanceSB>> m_tmp_instance_buffers_;
    tbb::concurrent_vector<DescriptorPtr> m_tmp_descriptor_heaps_;

    RenderMap m_render_candidates_[SHADER_DOMAIN_MAX];
  };
}
