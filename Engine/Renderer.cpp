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
  void Renderer::PreUpdate(const float& dt)
  {
    std::for_each
      (
       m_render_candidates_, m_render_candidates_ + SHADER_DOMAIN_MAX,
       [](auto& target_set) { target_set.clear(); }
      );

    m_tmp_descriptor_heaps_.clear();
    m_tmp_instance_buffers_.clear();
    m_b_ready_ = false;
  }

  void Renderer::Update(const float& dt) {}

  void Renderer::FixedUpdate(const float& dt) {}

  void Renderer::PreRender(const float& dt)
  {
    // Pre-processing, Mapping the materials to the model renderers.
    const auto& scene = GetSceneManager().GetActiveScene().lock();
    const auto& rcs = scene->GetCachedComponents<Components::Base::RenderComponent>();

    tbb::parallel_for_each
      (
       rcs.begin(), rcs.end(), [this](const WeakComponent& ptr_rc)
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
         case RENDER_COM_T_MODEL: preMappingModel(rc);
           break;
         case RENDER_COM_T_PARTICLE: preMappingParticle(rc);
           break;
         case RENDER_COM_T_UNK:
         default: break;
         }
       }
      );

    m_b_ready_ = true;
  }

  void Renderer::Render(const float& dt)
  {
    if (const auto scene = GetSceneManager().GetActiveScene().lock())
    {
      if (const auto cam = scene->GetMainCamera().lock())
      {
        const auto& cmd = GetD3Device().AcquireCommandPair(L"Main Rendering");

        cmd.SoftReset();

        for (auto i = 0; i < SHADER_DOMAIN_MAX; ++i)
        {
          // Check culling.
          RenderPass
            (
             dt, (eShaderDomain)i, false, cmd, [](const StrongObjectBase& obj)
             {
               return GetProjectionFrustum().CheckRender(obj);
             }, [i](const CommandPair& c, const DescriptorPtr& h)
             {
               GetRenderPipeline().DefaultRenderTarget(c);
               GetRenderPipeline().DefaultViewport(c);
               GetRenderPipeline().DefaultScissorRect(c);
               GetShadowManager().BindShadowMaps(c, h);
               GetShadowManager().BindShadowSampler(h);

               GetShadowManager().BindShadowMaps(cmd, heap);

               if (i > SHADER_DOMAIN_OPAQUE)
               {
                 GetReflectionEvaluator().BindReflectionMap(c, h);
               }

             },
             [i](const CommandPair& c, const DescriptorPtr& h)
             {
               GetShadowManager().UnbindShadowMaps(c);

               if (i > SHADER_DOMAIN_OPAQUE)
               {
                 GetReflectionEvaluator().UnbindReflectionMap(c);
               }
             }, m_additional_structured_buffers_
            );

          if (i == SHADER_DOMAIN_OPAQUE)
          {
            // Notify reflection evaluator that rendering is finished so that it
            // can copy the rendered scene to the copy texture.
            GetReflectionEvaluator().RenderFinished(cmd);
          }
        }
      }
    }
  }

  void Renderer::PostRender(const float& dt) {}

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
    const CommandPair&                                                   cmd,
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
    tbb::concurrent_hash_map<WeakMaterial, std::vector<SBs::InstanceSB>> final_mapping;

    for (const auto& mtr_m : target_set | std::views::values)
    {
      tbb::parallel_for_each
        (
         mtr_m.begin(), mtr_m.end(), [&](const CandidateTuple& tuple)
         {
           if (!predicate || predicate(std::get<0>(tuple).lock()))
           {
             decltype(final_mapping)::accessor acc;

             if (!final_mapping.find(acc, std::get<1>(tuple)))
             {
               final_mapping.insert(acc, std::get<1>(tuple));
             }

             acc->second.insert(acc->second.end(), std::get<2>(tuple).begin(), std::get<2>(tuple).end());
           }
         }
        );
    }

    if (!GetRenderPipeline().IsHeapAvailable())
    {
      throw std::runtime_error("Descriptor heap is not available!");
    }

    if (!GetD3Device().IsCommandPairAvailable())
    {
      throw std::runtime_error("Command pair is not available!");
    }

    for (const auto& [mtr, sbs] : final_mapping)
    {
      m_tmp_descriptor_heaps_.emplace_back(GetRenderPipeline().AcquireHeapSlot());

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

      renderPassImpl(dt, domain, shader_bypass, mtr.lock(), cmd, heap, sbs);

      for (const auto& sb : additional_structured_buffers)
      {
        if (const auto& sb_ptr = sb.lock())
        {
          sb_ptr->UnbindSRVGraphic(cmd);
        }
      }

      post_setup(cmd, heap);
    }
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
    StructuredBuffer<SBs::InstanceSB> instance_buffer;
    instance_buffer.Create(cmd.GetList(), structured_buffers.size(), structured_buffers.data());
    instance_buffer.BindSRVGraphic(cmd, heap);
    m_tmp_instance_buffers_.emplace_back(instance_buffer);

    material->SetTempParam
      (
       {
         .instanceCount = (UINT)structured_buffers.size(),
         .bypassShader = shader_bypass,
         .domain = domain,
       }
      );

    material->Draw(dt, cmd, heap);
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
        auto& domain_map = m_render_candidates_[domain];

        RenderMap::accessor acc;

        if (!domain_map.find(acc, RENDER_COM_T_MODEL))
        {
          domain_map.insert(acc, RENDER_COM_T_MODEL);
        }

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
        auto particles = pr->GetParticles();

        if (particles.empty()) { continue; }

        auto& domain_map = m_render_candidates_[domain];

        RenderMap::accessor acc;

        if (domain_map.find(acc, RENDER_COM_T_PARTICLE))
        {
          domain_map.insert(acc, RENDER_COM_T_PARTICLE);
        }

        tbb::concurrent_vector<SBs::InstanceSB> mp_particles(particles.begin(), particles.end());

        if (pr->IsFollowOwner())
        {
          for (auto& particle : particles)
          {
            Matrix mat = particle.GetParam<Matrix>(0);
            mat = tr->GetWorldMatrix().Transpose() * mat;
            particle.SetParam(0, mat);
          }
        }

        acc->second.push_back(std::make_tuple(obj, mtr, mp_particles));
      }
    }
  }
}
