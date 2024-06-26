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
    for (const auto& heap : m_tmp_descriptor_heaps_)
    {
      heap->Release();
    }

    m_tmp_descriptor_heaps_.clear();
    m_b_ready_ = false;
  }

  void Renderer::Update(const float& dt) {}

  void Renderer::FixedUpdate(const float& dt) {}

  void Renderer::PreRender(const float& dt)
  {
    // Pre-processing, Mapping the materials to the model renderers.
    const auto& scene = GetSceneManager().GetActiveScene().lock();
    BuildRenderMap(scene, m_render_candidates_, m_instance_count_);

    if (m_tmp_instance_buffers_.size() < m_instance_count_)
    {
      m_tmp_instance_buffers_.resize(m_instance_count_);
    }

    m_b_ready_ = true;
  }

  void Renderer::Render(const float& dt)
  {
    if (const auto scene = GetSceneManager().GetActiveScene().lock())
    {
      if (const auto cam = scene->GetMainCamera().lock())
      {
        const auto& cmd = GetD3Device().AcquireCommandPair(L"Main Rendering").lock();

        cmd->SoftReset();

        m_current_instance_ = 0;

        for (auto i = 0; i < SHADER_DOMAIN_MAX; ++i)
        {
          // Check culling.
          m_current_instance_ = RenderPass
            (
             dt, (eShaderDomain)i, false, cmd,
             m_current_instance_,
             m_tmp_descriptor_heaps_,
             m_tmp_instance_buffers_, [](const WeakObjectBase& obj)
             {
               return GetProjectionFrustum().CheckRender(obj);
             }, [i](const Weak<CommandPair>& c, const DescriptorPtr& h)
             {
               GetD3Device().DefaultRenderTarget(c);
               GetRenderPipeline().DefaultViewport(c);
               GetRenderPipeline().DefaultScissorRect(c);
               GetShadowManager().BindShadowMaps(c, h);
               GetShadowManager().BindShadowSampler(h);

               if (i > SHADER_DOMAIN_OPAQUE)
               {
                 GetReflectionEvaluator().BindReflectionMap(c, h);
               }
             }, [](const Weak<CommandPair>& c, const DescriptorPtr& h)
             {
               GetShadowManager().UnbindShadowMaps(c);
             }, m_additional_structured_buffers_
            );

          if (i == SHADER_DOMAIN_OPAQUE)
          {
            // Notify reflection evaluator that rendering is finished so that it
            // can copy the rendered scene to the copy texture.
            GetReflectionEvaluator().RenderFinished(cmd);
          }
        }

        GetReflectionEvaluator().UnbindReflectionMap(cmd);

        cmd->FlagReady();
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

  /**
   * \brief Render objects with the given domains.
   * \param dt Delta time.
   * \param domain Shader Domain.
   * \param shader_bypass not use the material shader if flagged
   * \param w_cmd Weak command pair pointer.
   * \param begin_idx start index of container iteration for storing instance data.
   * \param descriptor_heap_container heap containers for the each instance, will be pushed back.
   * \param instance_buffer_container instance container for the each instance, will be used index wise to optimize by not creating an instance buffer over and over again.
   * \param predicate filter any object if it is not satisfied.
   * \param initial_setup Initial setup for command list.
   * \param post_setup Post setup for command list.
   * \param additional_structured_buffers structured buffer to bind.
   * \return Returns the last index of the container.
   */
  UINT64 Renderer::RenderPass
  (
    const float                                                                dt,
    const eShaderDomain                                                        domain,
    bool                                                                       shader_bypass,
    const Weak<CommandPair>&                                                   w_cmd,
    const UINT64                                                               begin_idx,
    tbb::concurrent_vector<StrongDescriptorPtr>&                               descriptor_heap_container,
    tbb::concurrent_vector<StructuredBuffer<SBs::InstanceSB>>&                 instance_buffer_container,
    const std::function<bool(const StrongObjectBase&)>&                        predicate,
    const std::function<void(const Weak<CommandPair>&, const DescriptorPtr&)>& initial_setup,
    const std::function<void(const Weak<CommandPair>&, const DescriptorPtr&)>& post_setup,
    const std::vector<Weak<StructuredBufferBase>>&                             additional_structured_buffers = {}
  )
  {
    if (!Ready())
    {
      GetDebugger().Log("Renderer is not ready for rendering!");
      return begin_idx;
    }

    if (m_render_candidates_[domain].empty())
    {
      return begin_idx;
    }

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

    const auto& cmd = w_cmd.lock();

    for (const auto& sb_ptr : additional_structured_buffers)
    {
      if (const auto& sb = sb_ptr.lock())
      {
        sb->TransitionToSRV(cmd->GetList());
      }
    }

    UINT64 idx = begin_idx;

    for (const auto& [mtr, sbs] : final_mapping)
    {
      descriptor_heap_container.emplace_back(GetRenderPipeline().AcquireHeapSlot().lock());

      const auto& heap = descriptor_heap_container.back();

      initial_setup(cmd, heap);

      for (const auto& sb_ptr : additional_structured_buffers)
      {
        if (const auto& sb = sb_ptr.lock())
        {
          sb->CopySRVHeap(heap);
        }
      }

      heap->BindGraphic(cmd);

      renderPassImpl(dt, idx, domain, shader_bypass, instance_buffer_container, mtr.lock(), cmd, heap, sbs);

      post_setup(cmd, heap);

      ++idx;
    }

    for (const auto& sb_ptr : additional_structured_buffers)
    {
      if (const auto& sb = sb_ptr.lock())
      {
        sb->TransitionCommon(cmd->GetList(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
      }
    }

    return idx;
  }

  UINT64 Renderer::GetInstanceCount() const
  {
    if (!m_b_ready_)
    {
      throw std::runtime_error("Renderer is not ready for rendering!");
    }

    return m_instance_count_;
  }

  void Renderer::renderPassImpl(
    const float                                                dt,
    const UINT64                                               idx,
    eShaderDomain                                              domain,
    bool                                                       shader_bypass,
    tbb::concurrent_vector<StructuredBuffer<SBs::InstanceSB>>& instance_buffers,
    const StrongMaterial&                                      material,
    const Weak<CommandPair>&                                   w_cmd,
    const DescriptorPtr&                                       heap,
    const std::vector<SBs::InstanceSB>&                        structured_buffers
  )
  {
    const auto& cmd = w_cmd.lock();
    instance_buffers[idx].SetData(cmd->GetList(), static_cast<UINT>(structured_buffers.size()), structured_buffers.data());
    instance_buffers[idx].TransitionToSRV(cmd->GetList());
    instance_buffers[idx].CopySRVHeap(heap);

    material->SetTempParam
      (
       {
         .instanceCount = (UINT)structured_buffers.size(),
         .bypassShader = shader_bypass,
         .domain = domain,
       }
      );

    material->Draw(dt, cmd, heap);

    instance_buffers[idx].TransitionCommon(cmd->GetList(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
  }
}
