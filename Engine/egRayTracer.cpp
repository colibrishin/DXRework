#include "pch.h"
#include "egRayTracer.hpp"

#include "Renderer.h"
#include "egAnimator.h"
#include "egAtlasAnimation.h"
#include "egBoneAnimation.h"
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
  }

  void RayTracer::Render(const float& dt)
  {
    const auto& cmd = GetD3Device().AcquireCommandPair(L"Pre-processing TLAS").lock();

    cmd->SoftReset();

    RenderPass(cmd->GetList4(), nullptr);

    cmd->FlagReady([this]()
    {
      m_built_ = true;
      m_built_.notify_all();
    });
  }

  void RayTracer::PostRender(const float& dt) {}

  void RayTracer::PostUpdate(const float& dt) {}

  void RayTracer::Initialize() {}

  bool   RayTracer::Ready() const { return GetRenderer().Ready(); }

  UINT64 RayTracer::GetInstanceCount() const { return GetRenderer().GetInstanceCount(); }

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
