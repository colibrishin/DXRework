#include "pch.h"
#include "egParticleRenderer.h"
#include "egComputeShader.h"
#include "egImGuiHeler.hpp"
#include "egTransform.h"

SERIALIZER_ACCESS_IMPL
(
 Engine::Components::ParticleRenderer,
 _ARTAG(_BSTSUPER(RenderComponent))
 _ARTAG(m_cs_meta_path_str_)
 _ARTAG(m_instances_)
 _ARTAG(m_b_follow_owner_)
 _ARTAG(m_b_scaling_)
 _ARTAG(m_duration_dt_)
 _ARTAG(m_size_)
 _ARTAG(m_max_scale_size_)
 _ARTAG(m_min_scale_size_)
)

namespace Engine::Components
{
  ParticleRenderer::ParticleRenderer(const WeakObject& owner)
    : RenderComponent(RENDER_COM_T_PARTICLE, owner),
      m_b_follow_owner_(true),
      m_b_scaling_(false),
      m_duration_dt_(100.0f),
      m_size_(0.5f),
      m_max_scale_size_(1.f),
      m_min_scale_size_(1.f) {}

  void ParticleRenderer::Initialize()
  {
    m_sb_buffer_.Create(1, nullptr, true);
    LinearSpread(Vector3::One, Vector3::Zero);
  }

  void ParticleRenderer::Update(const float& dt)
  {
    if (m_cs_)
    {
      // Particle count
      GetRenderPipeline().SetParam((int)m_instances_.size(), particle_count_slot);
      // Current delta time
      GetRenderPipeline().SetParam(dt, dt_slot);
      // Max life time of a particle.
      GetRenderPipeline().SetParam(m_duration_dt_, duration_slot);

      // Scaling properties
      if (m_b_scaling_)
      {
        GetRenderPipeline().SetParam((int)m_b_scaling_, scaling_active_slot);
        GetRenderPipeline().SetParam(m_min_scale_size_, scaling_min_slot);
        GetRenderPipeline().SetParam(m_max_scale_size_, scaling_max_slot);
      }

      m_sb_buffer_.SetData(static_cast<UINT>(m_instances_.size()), m_instances_.data());
      m_sb_buffer_.BindUAV();

      const auto thread      = m_cs_->GetThread();
      const auto flatten     = thread[0] * thread[1] * thread[2];
      const UINT group_count = static_cast<UINT>(m_instances_.size() / flatten);
      const UINT remainder   = static_cast<UINT>(m_instances_.size() % flatten);

      m_cs_->SetGroup({group_count + (remainder ? 1 : 0), 1, 1});
      m_cs_->Dispatch();
      m_sb_buffer_.UnbindUAV();
      m_sb_buffer_.GetData(static_cast<UINT>(m_instances_.size()), m_instances_.data());
    }

    // Remove inactive particles.
    for (auto it = m_instances_.begin(); it != m_instances_.end();)
    {
      if (!it->GetActive()) { it = m_instances_.erase(it); }
      else { ++it; }
    }
  }

  void ParticleRenderer::PreUpdate(const float& dt) {}

  void ParticleRenderer::FixedUpdate(const float& dt) {}

  void ParticleRenderer::OnSerialized()
  {
    RenderComponent::OnSerialized();

    if (m_cs_)
    {
      Serializer::Serialize(m_cs_->GetName(), m_cs_);
      m_cs_meta_path_str_ = m_cs_->GetMetadataPath().string();
    }
  }

  void ParticleRenderer::OnDeserialized()
  {
    RenderComponent::OnDeserialized();

    if (const auto cs = Resources::ComputeShader::GetByMetadataPath(m_cs_meta_path_str_).lock())
    {
      m_cs_ = cs;
    }
  }

  void ParticleRenderer::OnImGui()
  {
    RenderComponent::OnImGui();

    CheckboxAligned("Follow Owner", m_b_follow_owner_);
    CheckboxAligned("Scaling Effect", m_b_scaling_);

    if (m_b_scaling_)
    {
      FloatAligned("Min Scale", m_min_scale_size_);
      FloatAligned("Max Scale", m_max_scale_size_);
    }

    FloatAligned("Duration", m_duration_dt_);
    FloatAligned("Size", m_size_);

    if (ImGui::Button("Spread Linearly"))
    {
      LinearSpread(Vector3::One, Vector3::Zero);
    }

    static UINT count = m_instances_.size();
    UINTAligned("Particle Count", count);

    if (ImGui::Button("Set Count"))
    {
      if (count != m_instances_.size())
      {
        SetCount(count);
      }
    }

    TextDisabled("Compute Shader", m_cs_meta_path_str_);
    if (ImGui::BeginDragDropTarget())
    {
      if (const auto payload = ImGui::AcceptDragDropPayload("RESOURCE"))
      {
        const StrongResource res = *static_cast<StrongResource*>(payload->Data);
        if (const auto cs = boost::dynamic_pointer_cast<Resources::ComputeShader>(res))
        {
          cs->Load();
          m_cs_               = cs;
          m_cs_meta_path_str_ = cs->GetMetadataPath().string();
        }
      }
      ImGui::EndDragDropTarget();
    }
  }

  const std::vector<Graphics::SBs::InstanceSB>& ParticleRenderer::GetParticles() const
  {
    return reinterpret_cast<const std::vector<Graphics::SBs::InstanceSB>&>(m_instances_);
  }

  void ParticleRenderer::SetFollowOwner(const bool follow) { m_b_follow_owner_ = follow; }

  void ParticleRenderer::SetCount(const size_t count)
  {
    // Expand and apply the world matrix of the owner to each instance.
    for (int i = 0; i < count; ++i)
    {
      Graphics::SBs::InstanceParticleSB sb;
      m_instances_.push_back(sb);
    }
  }

  void ParticleRenderer::SetDuration(const float duration) { m_duration_dt_ = duration; }

  void ParticleRenderer::LinearSpread(const Vector3& local_min, const Vector3& local_max)
  {
    const auto count = m_instances_.size();
    for (auto i = 0; i < count; ++i)
    {
      auto& instance = m_instances_[i];
      auto  world    = instance.GetWorld().Transpose();

      const auto new_pos = Vector3::Lerp(local_min, local_max, static_cast<float>(i) / static_cast<float>(count));

      world *= Matrix::CreateScale(m_size_) * Matrix::CreateTranslation(new_pos - world.Translation());
      instance.SetWorld(world.Transpose());
    }
  }

  bool ParticleRenderer::IsFollowOwner() const { return m_b_follow_owner_; }

  ParticleRenderer::ParticleRenderer()
    : RenderComponent(RENDER_COM_T_PARTICLE, {}),
      m_b_follow_owner_(true),
      m_b_scaling_(false),
      m_duration_dt_(100.0f),
      m_size_(0.5f),
      m_max_scale_size_(1.f),
      m_min_scale_size_(1.f) {}

  void ParticleRenderer::SetSize(const float size) { m_size_ = size; }

  void ParticleRenderer::SetComputeShader(const WeakComputeShader& cs)
  {
    if (const auto shader = cs.lock())
    {
      m_cs_ = shader;
      m_cs_meta_path_str_ = shader->GetMetadataPath().string();
    }
  }

  void ParticleRenderer::SetScaling(const bool scaling) { m_b_scaling_ = scaling; }

  void ParticleRenderer::SetScalingParam(const float min, const float max)
  {
    m_min_scale_size_ = min;
    m_max_scale_size_ = max;
  }
}
