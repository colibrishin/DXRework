#include "pch.h"
#include "Renderer.h"

#include "egAnimator.h"
#include "egAtlasAnimation.h"
#include "egBaseAnimation.h"
#include "egBoneAnimation.h"
#include "egMaterial.h"
#include "egModelRenderer.h"
#include "egParticleRenderer.h"
#include "egProjectionFrustum.h"
#include "egReflectionEvaluator.h"
#include "egSceneManager.hpp"
#include "egShape.h"
#include "egTexture.h"
#include "egTransform.h"

namespace Engine::Manager::Graphics
{
  void Renderer::PreUpdate(const float& dt) {}

  void Renderer::Update(const float& dt) {}

  void Renderer::FixedUpdate(const float& dt) {}

  void Renderer::PreRender(const float& dt)
  {
    // Pre-processing, Mapping the materials to the model renderers.
    const auto& scene = GetSceneManager().GetActiveScene().lock();
    const auto& rcs = scene->GetCachedComponents<Components::Base::RenderComponent>();

    for (const auto& ptr_rc : rcs)
    {
      // pointer sanity check
      if (ptr_rc.expired()) { continue; }
      const auto rc = ptr_rc.lock()->GetSharedPtr<Components::Base::RenderComponent>();

      // owner object sanity check
      const auto obj = rc->GetOwner().lock();
      if (!obj) { continue; }
      if (!obj->GetActive()) { continue; }

      // material sanity check
      const auto ptr_mtr = rc->GetMaterial();
      if (ptr_mtr.expired()) { continue; }
      const auto mtr = ptr_mtr.lock();

      // transform check, continue if it is disabled. (undefined behaviour)
      const auto tr = obj->GetComponent<Components::Transform>().lock();
      if (!tr) { continue; }
      if (!tr->GetActive()) { continue; }

      switch (rc->GetRenderType())
      {
      case RENDER_COM_T_MODEL:
          preMappingModel(rc);
          break;
      case RENDER_COM_T_PARTICLE:
          preMappingParticle(rc);
          break;
      case RENDER_COM_T_UNK:
      default: break;
      }
    }

    m_b_ready_ = true;
  }

  void Renderer::Render(const float& dt)
  {
    if (const auto scene = GetSceneManager().GetActiveScene().lock())
    {
      if (const auto cam = scene->GetMainCamera().lock())
      {
        for (auto i = 0; i < SHADER_DOMAIN_MAX; ++i)
        {
          // Check culling.
          RenderPass
            (
             dt, (eShaderDomain)i, false, [](const StrongObjectBase& obj)
             {
               return GetProjectionFrustum().CheckRender(obj);
             }, [i](const CommandPair& cmd, const DescriptorPtr& heap)
             {
               GetRenderPipeline().DefaultRenderTarget(cmd.GetList());
               GetRenderPipeline().DefaultViewport(cmd.GetList());
               GetRenderPipeline().DefaultScissorRect(cmd.GetList());

               GetShadowManager().BindShadowMaps(cmd, heap);

               if (i > SHADER_DOMAIN_OPAQUE)
               {
                 GetReflectionEvaluator().BindReflectionMap(cmd, heap);
               }

             }, [i](const CommandPair& cmd, const DescriptorPtr& heap)
             {
               GetShadowManager().UnbindShadowMaps(cmd);

               if (i > SHADER_DOMAIN_OPAQUE)
               {
                 GetReflectionEvaluator().UnbindReflectionMap(cmd);
               }
             },
             m_additional_structured_buffers_
            );

          if (i == SHADER_DOMAIN_OPAQUE)
          {
            // Notify reflection evaluator that rendering is finished so that it
            // can copy the rendered scene to the copy texture.
            GetReflectionEvaluator().RenderFinished();
          }
        }
      }
    }
  }

  void Renderer::PostRender(const float& dt)
  {
    std::for_each
      (
       m_render_candidates_, m_render_candidates_ + SHADER_DOMAIN_MAX,
       [](auto& target_set) { target_set.clear(); }
      );

    m_b_ready_ = false;
  }

  void Renderer::PostUpdate(const float& dt) {}

  void Renderer::Initialize() {}

  void Renderer::AppendAdditionalStructuredBuffer(const Weak<StructuredBufferBase>& sb_ptr)
  {
    if (const auto sb = sb_ptr.lock())
    {
      m_additional_structured_buffers_.push_back(sb);
    }
  }

  bool Renderer::Ready() const { return m_b_ready_; }

  void Renderer::RenderPass
  (
    const float                                                          dt,
    eShaderDomain                                                        domain,
    bool                                                                 shader_bypass,
    const std::function<bool(const StrongObjectBase&)>&                  predicate,
    const std::function<void(const CommandPair&, const DescriptorPtr&)>& initial_setup,
    const std::function<void(const CommandPair&, const DescriptorPtr&)>& post_setup,
    const std::vector<Weak<StructuredBufferBase>>&                       additional_structured_buffers = {}
  )
  {
    if (!Ready())
    {
      GetDebugger().Log("Renderer is not ready!");
      return;
    }

    if (m_render_candidates_[domain].empty()) { return; }

    const auto& target_set = m_render_candidates_[domain];
    std::map<WeakMaterial, std::vector<SBs::InstanceSB>> final_mapping;

    for (const auto& mtr_m : target_set | std::views::values)
    {
      for (const auto& [mtr, obj_v] : mtr_m)
      {
        for (const auto& [obj, sbs] : obj_v)
        {
          if (predicate && predicate(obj))
          {
            final_mapping[mtr].insert(final_mapping[mtr].end(), sbs.begin(), sbs.end());
          }
        }
      }
    }

    GetD3Device().Flush();

    std::vector<CommandPair> command_pairs;
    std::vector<DescriptorPtr> heaps;

    for (const auto& [mtr, sbs] : final_mapping)
    {
      if (!GetD3Device().IsCommandPairAvailable())
      {
        GetD3Device().Flush();
        heaps.clear();
      }

      const auto& cmd  = GetD3Device().AcquireCommandPair(L"Renderer Material Pass");

      command_pairs.emplace_back(cmd);
      heaps.emplace_back(GetRenderPipeline().AcquireHeapSlot());

      const auto& heap = heaps.back(); 

      cmd.SoftReset();

      for (const auto& sb_ptr : additional_structured_buffers)
      {
        if (const auto& sb = sb_ptr.lock())
        {
          sb->BindSRVGraphic(cmd, heap);
        }
      }

      renderPassImpl(dt, domain, shader_bypass, mtr.lock(), initial_setup, post_setup, cmd, heap, sbs);

      for (const auto& sb : additional_structured_buffers)
      {
        if (const auto& sb_ptr = sb.lock())
        {
          sb_ptr->UnbindSRVGraphic(cmd);
        }
      }
    }

    GetD3Device().Flush();

    m_tmp_instance_buffers_.clear();
  }

  void Renderer::renderPassImpl(
    const float                                                          dt,
    eShaderDomain                                                        domain,
    bool                                                                 shader_bypass,
    const StrongMaterial&                                                material,
    const std::function<void(const CommandPair&, const DescriptorPtr&)>& initial_setup,
    const std::function<void(const CommandPair&, const DescriptorPtr&)>& post_setup,
    const CommandPair&                                                   cmd,
    const DescriptorPtr&                                heap,
    const std::vector<SBs::InstanceSB>&                                  structured_buffers
  )
  {
    initial_setup(cmd, heap);

    m_tmp_instance_buffers_.push_back({});
    m_tmp_instance_buffers_.back().Create(structured_buffers.size(), structured_buffers.data());
    m_tmp_instance_buffers_.back().BindSRVGraphic(cmd, heap);

    material->SetTempParam
      (
       {
         .instanceCount = (UINT)structured_buffers.size(),
         .bypassShader = shader_bypass,
         .domain = domain,
       }
      );

    material->Draw(dt, cmd, heap);

    post_setup(cmd, heap);
  }

  void Renderer::preMappingModel(const StrongRenderComponent& rc)
  {
    // get model renderer, continue if it is disabled
    const auto mr = rc->GetSharedPtr<Components::ModelRenderer>();
    if (!mr->GetActive()) { return; }

    const auto obj = rc->GetOwner().lock();
    const auto mtr = rc->GetMaterial().lock();
    const auto tr = obj->GetComponent<Components::Transform>().lock();

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
        AtlasAnimationPrimitive::AtlasFramePrimitive atlas_frame;
        atlas_anim->GetFrame(anim_frame, atlas_frame);

        atlas_x                 = atlas_frame.X;
        atlas_y                 = atlas_frame.Y;
        atlas_w                 = atlas_frame.Width;
        atlas_h                 = atlas_frame.Height;
      }
    }

    // Pre-mapping by the material.
    for (auto i = 0; i < SHADER_DOMAIN_MAX; ++i)
    {
      const auto domain = static_cast<eShaderDomain>(i);

      if (mtr->IsRenderDomain(domain))
      {
        auto& target_set = m_render_candidates_[domain][RENDER_COM_T_MODEL][mtr];

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
        target_set.push_back({obj, {std::move(sb)}});
      }
    }
  }

  void Renderer::preMappingParticle(const StrongRenderComponent& rc)
  {
    // get model renderer, continue if it is disabled
    const auto pr = rc->GetSharedPtr<Components::ParticleRenderer>();
    if (!pr->GetActive()) { return; }

    const auto obj = rc->GetOwner().lock();
    const auto mtr = rc->GetMaterial().lock();
    const auto tr = obj->GetComponent<Components::Transform>().lock();

    // Pre-mapping by the material.
    for (auto i = 0; i < SHADER_DOMAIN_MAX; ++i)
    {
      const auto domain = static_cast<eShaderDomain>(i);

      if (mtr->IsRenderDomain(domain))
      {
        auto& target_set = m_render_candidates_[domain][RENDER_COM_T_PARTICLE][mtr];
        auto particles = pr->GetParticles();

        if (pr->IsFollowOwner())
        {
          for (auto& particle : particles)
          {
            Matrix mat = particle.GetParam<Matrix>(0);
            mat = tr->GetWorldMatrix().Transpose() * mat;
            particle.SetParam(0, mat);
          }
        }

        if (particles.empty()) { continue; }

        target_set.push_back({obj, particles});
      }
    }
  }
}
