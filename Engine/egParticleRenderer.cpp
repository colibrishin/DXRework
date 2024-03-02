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
)

namespace Engine::Components
{
  ParticleRenderer::ParticleRenderer(const WeakObject& owner)
    : RenderComponent(RENDER_COM_T_PARTICLE, owner),
      m_b_follow_owner_(true) {}

  void ParticleRenderer::Initialize()
  {
    m_sb_buffer_.Create(1, nullptr, true);
    SetCount(1);
  }

  void ParticleRenderer::Update(const float& dt)
  {
    if (m_cs_ && GetMaterial().lock())
    {
      const auto& ticket = GetRenderPipeline().SetParam(m_params_);

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

    FloatAligned("Duration", m_params_.GetParam<float>(duration_slot));
    FloatAligned("Size", m_params_.GetParam<float>(size_slot));
    UINTAligned("Count", m_params_.GetParam<UINT>(particle_count_slot));

    if (ImGui::Button("Set Count"))
    {
      if (m_params_.GetParam<UINT>(size_slot) != m_instances_.size())
      {
        SetCount(m_params_.GetParam<UINT>(size_slot));
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

    if (m_cs_)
    {
      m_cs_->OnImGui(GetSharedPtr<ParticleRenderer>());
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

  void ParticleRenderer::SetDuration(const float duration)
  {
    m_params_.SetParam(duration, duration_slot);

    for (auto& instance : m_instances_)
    {
      instance.SetLife(duration);
    }
  }

  void ParticleRenderer::SetSize(const float size)
  {
    m_params_.SetParam(size, size_slot);
  }

  bool ParticleRenderer::IsFollowOwner() const { return m_b_follow_owner_; }

  ParticleRenderer::ParticleRenderer()
    : RenderComponent(RENDER_COM_T_PARTICLE, {}),
      m_b_follow_owner_(true) {}

  void ParticleRenderer::SetComputeShader(const WeakComputeShader& cs)
  {
    if (const auto shader = cs.lock())
    {
      m_cs_ = shader;
      m_cs_meta_path_str_ = shader->GetMetadataPath().string();
    }
  }
}
