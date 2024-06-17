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
  }

  void RayTracer::Render(const float& dt) {}

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
      // Scrap the BLAS.
      std::map<WeakModel, std::vector<SBs::InstanceSB>> target_instances;

      // todo: opaque only.
      for (const auto& candidates : GetRenderer().m_render_candidates_[0] | std::views::values)
      {
        for (const auto& candidate : candidates)
        {
          const auto& obj       = std::get<0>(candidate).lock();
          const auto& mtr       = std::get<1>(candidate).lock();
          const auto& instances = std::get<2>(candidate);

          if (predicate && !predicate(obj)) { continue; }

          if (const auto& shape = mtr->GetResource<Resources::Shape>(0).lock())
          {
            target_instances[shape].insert(target_instances[shape].end(), instances.begin(), instances.end());
          }
        }
      }

      // Build the TLAS based on the target instances.
      GetRaytracingPipeline().BuildTLAS(cmd, target_instances);

      cmd->SetComputeRootSignature(GetRenderPipeline().GetRootSignature());

      // set root signature
      // set descriptor heaps
      // bind the pipeline state object => shader required
      // bind the tlas
      
    }
  }
}
