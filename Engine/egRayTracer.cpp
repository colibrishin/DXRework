#include "pch.h"
#include "egRayTracer.hpp"

#include "Renderer.h"
#include "egAnimator.h"
#include "egAtlasAnimation.h"
#include "egBoneAnimation.h"
#include "egLight.h"
#include "egParticleRenderer.h"
#include "egRaytracingPipeline.hpp"
#include "egSceneManager.hpp"
#include "egShape.h"
#include "egTransform.h"

namespace Engine::Manager::Graphics
{
  void RayTracer::PreUpdate(const float& dt) {}
  void RayTracer::Update(const float& dt) {}
  void RayTracer::FixedUpdate(const float& dt) {}

  void RayTracer::PreRender(const float& dt)
  {
    // Reuse the renderer candidates;
    m_built_ = false;

    m_light_buffers_.clear();

    if (const auto& scene = GetSceneManager().GetActiveScene().lock())
    {
      const auto& lights = (*scene)[LAYER_LIGHT];

      for (const auto& w_obj : lights->GetGameObjects())
      {
        if (const auto locked = w_obj.lock())
        {
          const auto& light = locked->GetSharedPtr<Objects::Light>();
          const auto  tr    = locked->GetComponent<Components::Transform>().lock();

          const auto world = tr->GetWorldMatrix();

          m_light_buffers_.emplace_back
            (
             tr->GetWorldMatrix().Transpose(),
             light->GetColor(),
             light->GetType(),
             light->GetRange()
            );
        }
      }
    }
  }

  void RayTracer::Render(const float& dt)
  {
    const auto& cmd = GetD3Device().AcquireCommandPair(L"Pre-processing TLAS").lock();

    cmd->SoftReset();

    m_light_buffer_data_.SetData(cmd->GetList4(), m_light_buffers_.size(), m_light_buffers_.data());

    RenderPass(cmd->GetList4(), nullptr);

    cmd->Execute();
  }

  void RayTracer::PostRender(const float& dt) {}

  void RayTracer::PostUpdate(const float& dt) {}

  void RayTracer::Initialize()
  {
    const auto& cmd = GetD3Device().AcquireCommandPair(L"RayTracer Initialization").lock();

    cmd->SoftReset();

    m_light_buffer_data_.Create(cmd->GetList(), 1, {});

    cmd->FlagReady();
  }

  bool   RayTracer::Ready() const { return GetRenderer().Ready(); }

  UINT64 RayTracer::GetInstanceCount() const { return GetRenderer().GetInstanceCount(); }

  const Graphics::StructuredBuffer<Graphics::SBs::LightSB>& RayTracer::GetLightSB() const
  {
    return m_light_buffer_data_;
  }

  void RayTracer::RenderPass
  (
    ID3D12GraphicsCommandList4*                         cmd,
    const std::function<bool(const StrongObjectBase&)>& predicate
  )
  {
    if (const auto& scene = GetSceneManager().GetActiveScene().lock())
    {
      UINT32 total_item_count = 0;

      // Scrap the BLAS.
      std::map<WeakMaterial, std::vector<SBs::InstanceSB>> target_instances;

      // todo: opaque only.
      for (const auto& candidates : GetRenderer().m_render_candidates_[0] | std::views::values)
      {
        for (const auto& candidate : candidates)
        {
          const auto& obj       = std::get<0>(candidate).lock();
          const auto& mtr       = std::get<1>(candidate).lock();
          const auto& instances = std::get<2>(candidate);

          if (predicate && !predicate(obj)) { continue; }

          target_instances[mtr].insert(target_instances[mtr].end(), instances.begin(), instances.end());
          total_item_count += instances.size();
        }
      }

      if (total_item_count > m_tmp_instances_.size())
      {
        m_tmp_instances_.resize(total_item_count);
      }

      // Build the TLAS based on the target instances.
      GetRaytracingPipeline().BuildTLAS(cmd, target_instances, m_tmp_instances_);
    }
  }
}
