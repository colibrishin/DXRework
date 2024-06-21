#include "pch.h"
#include "egCommonRenderer.h"

#include "egAnimator.h"
#include "egAtlasAnimation.h"
#include "egBoneAnimation.h"
#include "egMaterial.h"
#include "egModelRenderer.h"
#include "egParticleRenderer.h"
#include "egRenderComponent.h"
#include "egSceneManager.hpp"
#include "egTransform.h"

namespace Engine::Manager::Graphics
{
  void __fastcall BuildRenderMap(const WeakScene& w_scene, RenderMap out_map[SHADER_DOMAIN_MAX], std::atomic<UINT64>& instance_count)
  {
    instance_count = 0;

    for (auto i = 0; i < SHADER_DOMAIN_MAX; ++i)
    {
      out_map[i].clear();
    }

    // Pre-processing, Mapping the materials to the model renderers.
    if (const auto& scene = w_scene.lock())
    {
      const auto& rcs = scene->GetCachedComponents<Components::Base::RenderComponent>();

      tbb::parallel_for_each
        (
         rcs.begin(), rcs.end(), [&out_map, &instance_count](const WeakComponent& ptr_rc)
         {
           // pointer sanity check
           if (ptr_rc.expired()) { return; }
           const auto rc = ptr_rc.lock()->GetSharedPtr<Components::Base::RenderComponent>();

           // owner object sanity check
           const auto obj = rc->GetOwner().lock();
           if (!obj) { return; }
           if (!obj->GetActive()) { return; }

           // material sanity check
           const auto ptr_mtr = rc->GetMaterial();
           if (ptr_mtr.expired()) { return; }
           const auto mtr = ptr_mtr.lock();

           // transform check, continue if it is disabled. (undefined behaviour)
           const auto tr = obj->GetComponent<Components::Transform>().lock();
           if (!tr) { return; }
           if (!tr->GetActive()) { return; }

           switch (rc->GetRenderType())
           {
           case RENDER_COM_T_MODEL: PremapModelImpl(rc->GetSharedPtr<Components::ModelRenderer>(), out_map, instance_count);
             break;
           case RENDER_COM_T_PARTICLE: PremapParticleImpl(rc->GetSharedPtr<Components::ParticleRenderer>(), out_map, instance_count);
             break;
           case RENDER_COM_T_UNK:
           default: break;
           }
         }
        );
    }
  }

  void PremapModelImpl(const WeakModelRenderer& model_component, RenderMap out_map[SHADER_DOMAIN_MAX], std::atomic<UINT64>& instance_count)
  {
    if (const auto& mr = model_component.lock())
    {
      // get model renderer, continue if it is disabled
      if (!mr->GetActive()) { return; }

      const auto obj = mr->GetOwner().lock();
      const auto mtr = mr->GetMaterial().lock();
      const auto tr  = obj->GetComponent<Components::Transform>().lock();

      // animator parameters
      float anim_frame    = 0.0f;
      UINT  anim_idx      = 0;
      UINT  anim_duration = 0;
      bool  no_anim       = false;
      UINT  atlas_x       = 0;
      UINT  atlas_y       = 0;
      UINT  atlas_w       = 0;
      UINT  atlas_h       = 0;

      if (const auto atr = obj->GetComponent<Components::Animator>().lock())
      {
        anim_frame = atr->GetFrame();
        anim_idx   = atr->GetAnimation();
        no_anim    = !atr->GetActive();

        if (const auto bone_anim = mtr->GetResource<Resources::BoneAnimation>(anim_idx).lock())
        {
          anim_duration = bone_anim->GetDuration();
        }

        if (const auto atlas_anim = mtr->GetResource<Resources::AtlasAnimation>(anim_idx).lock())
        {
          AtlasAnimationPrimitive::AtlasFramePrimitive atlas_frame{};
          atlas_anim->GetFrame(anim_frame, atlas_frame);

          atlas_x = atlas_frame.X;
          atlas_y = atlas_frame.Y;
          atlas_w = atlas_frame.Width;
          atlas_h = atlas_frame.Height;
        }
      }

      // Pre-mapping by the material.
      for (auto i = 0; i < SHADER_DOMAIN_MAX; ++i)
      {
        const auto domain = static_cast<eShaderDomain>(i);

        if (mtr->IsRenderDomain(domain))
        {
          auto& domain_map = out_map[domain];

          RenderMap::accessor acc;

          if (!domain_map.find(acc, RENDER_COM_T_MODEL)) { domain_map.insert(acc, RENDER_COM_T_MODEL); }

          SBs::InstanceModelSB sb{};
          sb.SetWorld(tr->GetWorldMatrix().Transpose());
          sb.SetFrame(anim_frame);
          sb.SetAnimDuration(anim_duration);
          sb.SetAnimIndex(anim_idx);
          sb.SetNoAnim(no_anim);
          sb.SetAtlasX(atlas_x);
          sb.SetAtlasY(atlas_y);
          sb.SetAtlasW(atlas_w);
          sb.SetAtlasH(atlas_h);

          // todo: stacking structured buffer data might be get large easily.
          acc->second.push_back(std::make_tuple(obj, mtr, tbb::concurrent_vector<SBs::InstanceSB>{std::move(sb)}));
          instance_count.fetch_add(1);
        }
      }
    }
  }

  void PremapParticleImpl(const WeakParticleRenderer& particle_component, RenderMap out_map[SHADER_DOMAIN_MAX], std::atomic<UINT64>& instance_count)
  {
    if (const auto& pr = particle_component.lock())
    {
      if (!pr->GetActive()) { return; }

      const auto obj = pr->GetOwner().lock();
      const auto mtr = pr->GetMaterial().lock();
      const auto tr  = obj->GetComponent<Components::Transform>().lock();

      // Pre-mapping by the material.
      for (auto i = 0; i < SHADER_DOMAIN_MAX; ++i)
      {
        const auto domain = static_cast<eShaderDomain>(i);

        if (mtr->IsRenderDomain(domain))
        {
          auto particles = pr->GetParticles();

          if (particles.empty()) { continue; }

          auto& domain_map = out_map[domain];

          RenderMap::accessor acc;

          if (!domain_map.find(acc, RENDER_COM_T_PARTICLE)) { domain_map.insert(acc, RENDER_COM_T_PARTICLE); }

          tbb::concurrent_vector<SBs::InstanceSB> mp_particles(particles.begin(), particles.end());

          if (pr->IsFollowOwner())
          {
            for (auto& particle : particles)
            {
              Matrix mat = particle.GetParam<Matrix>(0);
              mat        = tr->GetWorldMatrix().Transpose() * mat;
              particle.SetParam(0, mat);
            }
          }

          acc->second.push_back(std::make_tuple(obj, mtr, mp_particles));
          instance_count.fetch_add(particles.size());
        }
      }
    }
  }
}
