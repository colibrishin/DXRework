#include "pch.h"
#include "egParticleRenderer.h"
#include "egComputeShader.h"

SERIALIZER_ACCESS_IMPL
(
 Engine::Components::ParticleRenderer,
 _ARTAG(_BSTSUPER(RenderComponent))
 _ARTAG(m_cs_name_)
 _ARTAG(m_sbs_)
)

namespace Engine::Components
{
  ParticleRenderer::ParticleRenderer(const WeakObject& owner): RenderComponent(RENDER_COM_T_PARTICLE, owner) {}

  void ParticleRenderer::Initialize() {
    m_sb_buffer_.Create(1, nullptr, true);
    Spread(Vector3::One, Vector3::Zero);
  }

  void ParticleRenderer::Update(const float& dt) {
    if (m_cs_)
    {
      GetRenderPipeline().SetParam((int)m_sbs_.size(), particle_count_slot);
      GetRenderPipeline().SetParam(dt, dt_slot);
      m_sb_buffer_.SetData(m_sbs_.size(), reinterpret_cast<const Graphics::SBs::InstanceParticleSB*>(m_sbs_.data()));
      m_sb_buffer_.BindUAV();

      const auto thread      = m_cs_->GetThread();
      const auto flatten     = thread[0] * thread[1] * thread[2];
      const UINT group_count = m_sbs_.size() / flatten;
      const UINT remainder   = m_sbs_.size() % flatten;

      m_cs_->SetGroup({group_count + (remainder ? 1 : 0), 1, 1});
      m_cs_->Dispatch();
      m_sb_buffer_.UnbindUAV();
      m_sb_buffer_.GetData(m_sbs_.size(), reinterpret_cast<Graphics::SBs::InstanceParticleSB*>(m_sbs_.data()));
    }
  }

  void ParticleRenderer::PreUpdate(const float& dt) {}

  void ParticleRenderer::FixedUpdate(const float& dt) {}

  const std::vector<Graphics::SBs::InstanceSB>& ParticleRenderer::GetParticles() const { return m_sbs_; }

  void ParticleRenderer::SetCount(const size_t count) { m_sbs_.resize(count, {}); }

  void ParticleRenderer::Spread(const Vector3& local_min, const Vector3& local_max)
  {
    const auto count = m_sbs_.size();
    for (auto i = 0; i < count; ++i)
    {
      auto* sb    = reinterpret_cast<Graphics::SBs::InstanceParticleSB*>(&m_sbs_[i]);
      auto  world = sb->GetLocal();

      const auto new_pos = Vector3::Lerp(local_min, local_max, static_cast<float>(i) / static_cast<float>(count));

      world *= Matrix::CreateTranslation(new_pos - world.Translation());
      sb->SetLocal(world);
    }
  }

  ParticleRenderer::ParticleRenderer() : RenderComponent(RENDER_COM_T_PARTICLE, {}) {}

  void ParticleRenderer::SetComputeShader(const WeakComputeShader& cs)
  {
    if (const auto shader = cs.lock())
    {
      m_cs_ = shader;
    }
  }
}
