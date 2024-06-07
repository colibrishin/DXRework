#include "pch.h"
#include "clShadowIntersectionScript.h"

#include <Renderer.h>
#include <egMaterial.h>
#include <egRenderPipeline.h>
#include <egSceneManager.hpp>
#include <egShader.hpp>
#include <egTransform.h>

#include "egCamera.h"
#include "egMouseManager.h"

namespace Client::Scripts
{
  SCRIPT_CLONE_IMPL(ShadowIntersectionScript)

  void ShadowIntersectionScript::Initialize()
  {
	  Script::Initialize();

    m_shadow_shader_ = Resources::Shader::Get("cascade_shadow_stage1").lock();
    m_intensity_test_shader_ = Resources::Shader::Get("intensity_test").lock();

    if (const auto& cs = Engine::Resources::ComputeShader::Get("IntersectionCompute").lock())
    {
      m_intersection_compute_ = cs;
    }
    else
    {
      const auto& new_cs = Engine::Resources::ComputeShader::Create<ComputeShaders::IntersectionCompute>().lock();

      m_intersection_compute_ = new_cs;
    }

    m_tmp_shadow_depth_ = Engine::Resources::Texture2D::Create
      (
       std::to_string(GetID()) + "ShadowDepth",
       "",
       {
         .Alignment = 0,
         .Width = g_max_shadow_map_size,
         .Height = g_max_shadow_map_size,
         .DepthOrArraySize = 1,
         .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
         .Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
         .MipsLevel = 1,
         .Layout = D3D12_TEXTURE_LAYOUT_64KB_STANDARD_SWIZZLE,
         .SampleDesc = {1, 0},
       }
      );

    m_tmp_shadow_depth_->Initialize();
    m_tmp_shadow_depth_->Load();

    for (auto& tex : m_shadow_texs_)
    {
      tex.Initialize();
      tex.Load();
    }

    for (auto& tex : m_intensity_test_texs_)
    {
      tex.Initialize();
      tex.Load();
    }

    for (auto& tex : m_intensity_position_texs_)
    {
      tex.Initialize();
      tex.Load();
    }

    for (auto& tex : m_shadow_mask_texs_)
    {
      tex.Initialize();
      tex.Load();
    }

    m_sb_light_vp_ = boost::make_shared<Graphics::StructuredBuffer<Graphics::SBs::LightVPSB>>();
    m_sb_light_table_ = boost::make_shared<Graphics::StructuredBuffer<ComputeShaders::IntersectionCompute::LightTableSB>>();

    m_sb_light_vp_->Create(g_max_lights, nullptr);
    m_sb_light_table_->Create(g_max_lights, nullptr);

    m_viewport_.Width    = g_max_shadow_map_size;
    m_viewport_.Height   = g_max_shadow_map_size;
    m_viewport_.MinDepth = 0.0f;
    m_viewport_.MaxDepth = 1.0f;
    m_viewport_.TopLeftX = 0.0f;
    m_viewport_.TopLeftY = 0.0f;

    // also borrow the sampler state from shadow manager.
  }

  void ShadowIntersectionScript::PreUpdate(const float& dt) {}

  void ShadowIntersectionScript::Update(const float& dt)
  {
    Vector3 position;
    Vector3 dir;

    if (const auto& scene = GetSceneManager().GetActiveScene().lock())
    {
      if (const auto& camera = scene->GetMainCamera().lock())
      {
        position = camera->GetComponent<Components::Transform>().lock()->GetWorldPosition();
        dir      = camera->GetComponent<Components::Transform>().lock()->Forward();
      }
    }

    for (const auto& [key, bbox] : m_shadow_bbox_)
    {
      float dist = 0.f;

      if (bbox.Intersects(position, dir, dist))
      {
        GetDebugger().Draw(bbox, Colors::YellowGreen);
      }
      else
      {
        GetDebugger().Draw(bbox, Colors::Red);
      }
    }

    m_shadow_bbox_.clear();
  }

  void ShadowIntersectionScript::PostUpdate(const float& dt) {}

  void ShadowIntersectionScript::FixedUpdate(const float& dt) {}

  void ShadowIntersectionScript::PreRender(const float& dt) {}

  void ShadowIntersectionScript::Render(const float& dt) {}

  void ShadowIntersectionScript::PostRender(const float& dt)
  {
   // GetD3Device().WaitAndReset(COMMAND_LIST_UPDATE);

   // const auto& up_cmd = GetD3Device().GetCommandList(COMMAND_LIST_UPDATE);

   // constexpr float clear_color[4] = { 0.f, 0.f, 0.f, 0.f };

	  //for (auto& tex : m_shadow_texs_)
   // {
   //   tex.Clear(up_cmd);
   // }

   // for (auto& tex : m_intensity_position_texs_)
   // {
   //   // transition

   //   up_cmd->ClearRenderTargetView
   //   (
   //       tex.GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart(),
   //       clear_color,
   //       0,
   //       nullptr
   //   );
   // }

   // for (auto& tex : m_intensity_test_texs_)
   // {
   //   // transition

   //   up_cmd->ClearRenderTargetView
   //   (
   //       tex.GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart(),
   //       clear_color,
   //       0,
   //       nullptr
   //   );
   // }

   // for (auto& tex : m_shadow_mask_texs_)
   // {
   //   // transition

   //   up_cmd->ClearRenderTargetView
   //   (
   //       tex.GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart(),
   //       clear_color,
   //       0,
   //       nullptr
   //   );
   // }

   // DX::ThrowIfFailed(up_cmd->Close());

   // ID3D12CommandList* cmd_list[]
   // {
   //   up_cmd
   // };

   // GetD3Device().GetCommandQueue(COMMAND_LIST_UPDATE)->ExecuteCommandLists(1, cmd_list);

   // GetD3Device().Signal(COMMAND_TYPE_DIRECT);

   // GetD3Device().Wait();

   // std::vector<Graphics::SBs::LightVPSB> light_vps;
   // light_vps.reserve(g_max_lights);

   // // Build shadow map of this object for each light.
   // if (const auto& scene = GetOwner().lock()->GetScene().lock())
   // {
   //   constexpr size_t shadow_slot = 1;
   //   const auto       lights      = (*scene)[LAYER_LIGHT];

   //   for (const auto& light : *lights)
   //   {
   //     const auto& tr = light->GetComponent<Components::Transform>().lock();

   //     Vector3 light_dir;
   //     (tr->GetWorldPosition()).Normalize(light_dir);

   //     Graphics::SBs::LightVPSB light_vp{};
   //     GetShadowManager().EvalShadowVP(scene->GetMainCamera(), light_dir, light_vp);

   //     light_vps.push_back(light_vp);
   //   }

   //   m_sb_light_vp_->SetData(light_vps.size(), light_vps.data());

   //   UINT idx = 0;

   //   // Draw shadow map except the object itself.
   //   for (const auto& light : *lights)
   //   {
   //     GetRenderPipeline().SetParam<int>(idx, shadow_slot);

   //     GetRenderer().RenderPass
   //       (
   //        dt, SHADER_DOMAIN_OPAQUE, true,
   //        [this](const StrongObjectBase& obj)
   //        {
   //          if (obj->GetID() == GetOwner().lock()->GetID())
   //          {
   //            return false;
   //          }

   //          return true;
   //        }, [this, idx](const CommandPair& cmd, const DescriptorPtr& heap)
   //        {
   //          cmd.GetList()->RSSetViewports(1, &m_viewport_);

   //          cmd.GetList()->SetPipelineState(m_shadow_shader_->GetPipelineState());

   //          m_shadow_texs_[idx].Bind(cmd, heap, BIND_TYPE_RTV, 0, 0);

   //        }, [this, idx](const CommandPair& cmd, const DescriptorPtr& heap)
   //        {
   //          m_shadow_texs_[idx].Unbind(cmd, BIND_TYPE_RTV);
   //        }, { m_sb_light_vp_ }
   //       );

   //     ++idx;
   //   }

   //   idx = 0;

   //   // Render object only for picking up the exact shadow position.
   //   for (const auto& light : *lights)
   //   {
   //     ID3D11RenderTargetView* previous_rtv = nullptr;
   //     ID3D11DepthStencilView* previous_dsv = nullptr;

   //     const auto& prev_rtv = GetRenderPipeline().SetRenderTargetDeferred
   //       (
   //        COMMAND_LIST_POST_RENDER,
   //        m_shadow_mask_texs_[idx].GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart(), m_shadow_texs_[idx].GetDSVDescriptor()->GetCPUDescriptorHandleForHeapStart()
   //       );

   //     GetRenderPipeline().SetParam<int>(idx, shadow_slot);

   //     GetRenderer().RenderPass
   //       (
   //        dt, SHADER_DOMAIN_OPAQUE, true,
   //        [this](const StrongObjectBase& obj)
   //        {
   //          if (obj->GetID() != GetOwner().lock()->GetID())
   //          {
   //            return false;
   //          }

   //          return true;
   //        }, TODO, TODO, TODO
   //       );

   //     GetRenderPipeline().SetRenderTargetDeferred(COMMAND_LIST_POST_RENDER, prev_rtv);

   //     idx++;
   //   }

   //   m_sb_light_vp_->BindSRVGraphic(TODO, TODO);

   //   GetRenderPipeline().SetParam<int>(0, shadow_slot);

   //   GetRenderPipeline().DefaultViewport(COMMAND_LIST_POST_RENDER);
   //   GetRenderPipeline().DefaultRenderTarget(COMMAND_LIST_POST_RENDER);
   //   GetRenderPipeline().FallbackPSO(COMMAND_LIST_POST_RENDER);

   //   std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> shadow_srv;
   //   shadow_srv.reserve(g_max_lights);

   //   for (const auto& tex : m_shadow_texs_)
   //   {
   //     shadow_srv.push_back(tex.GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart());
   //   }

   //   heap.SetShaderResources
   //     (
   //      RESERVED_SHADOW_MAP,
   //      shadow_srv.size(),
   //      shadow_srv
   //     );

   //   m_sb_light_vp_.BindSRVGraphic(cmd, heap);
   //   GetRenderPipeline().SetPSO(m_intensity_test_shader_, COMMAND_LIST_POST_RENDER);

   //   constexpr size_t target_light_slot = 2;
   //   constexpr size_t custom_vp_slot    = 3;

   //   constexpr size_t custom_view_slot = 1;
   //   constexpr size_t custom_proj_slot = 2;

   //   // In light VP Render objects,
   //   // In pixel shader, while sampling shadow factor, do not use own shadow map
   //   // If shadow factor is above 0.f, and light intensity is above 0.f
   //   // then it intersects with light and shadow
   //   // Fill the pixel with light index (Do not use the sampler state, need raw value)

   //   GetRenderPipeline().SetViewportDeferred(COMMAND_LIST_POST_RENDER, m_viewport_);

   //   int z_clip = 0;

   //   if (const auto camera = scene->GetMainCamera().lock())
   //   {
   //     const float z = camera->GetComponent<Components::Transform>().lock()->GetWorldPosition().z;

   //     for (int i = 0; i < 3; ++i)
   //     {
   //       if (light_vps[0].end_clip_spaces[i].z > z )
   //       {
   //         z_clip = i;
   //         break;
   //       }
   //     }
   //   }

   //   std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> srv_ptr;

   //   for (auto& tex : m_shadow_mask_texs_)
   //   {
   //     srv_ptr.push_back(tex.GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart());
   //   }

   //   GetRenderPipeline().GetDescriptor().SetShaderResources
   //     (
   //      BIND_SLOT_TEXARR,
   //      static_cast<UINT>(srv_ptr.size()),
   //      srv_ptr
   //     );

   //   for (int i = 0; i < lights->size(); ++i)
   //   {
   //     GetRenderPipeline().SetParam<int>(i, target_light_slot);
   //     GetRenderPipeline().SetParam<int>(true, custom_vp_slot);

   //     GetRenderPipeline().SetParam<Matrix>(light_vps[i].view[z_clip], custom_view_slot);
   //     GetRenderPipeline().SetParam<Matrix>(light_vps[i].proj[z_clip], custom_proj_slot);

   //     std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtv_ptr;
   //     rtv_ptr.push_back(m_intensity_test_texs_[i].GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart());
   //     rtv_ptr.push_back(m_intensity_position_texs_[i].GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart());

   //     const auto& test_transition = CD3DX12_RESOURCE_BARRIER::Transition
   //       (
   //        m_intensity_test_texs_[i].GetRawResoruce(),
   //        D3D12_RESOURCE_STATE_COMMON,
   //        D3D12_RESOURCE_STATE_RENDER_TARGET
   //       );

   //     const auto& position_transition = CD3DX12_RESOURCE_BARRIER::Transition
   //       (
   //        m_intensity_position_texs_[i].GetRawResoruce(),
   //        D3D12_RESOURCE_STATE_COMMON,
   //        D3D12_RESOURCE_STATE_RENDER_TARGET
   //       );

   //     GetD3Device().GetCommandList(COMMAND_LIST_POST_RENDER)->ResourceBarrier(1, &test_transition);
   //     GetD3Device().GetCommandList(COMMAND_LIST_POST_RENDER)->ResourceBarrier(1, &position_transition);

   //     const auto& prev_rtv = GetRenderPipeline().SetRenderTargetDeferred
   //       (
   //        COMMAND_LIST_POST_RENDER,
   //        rtv_ptr.size(),
   //        rtv_ptr.data(), m_tmp_shadow_depth_->GetDSVDescriptor()->GetCPUDescriptorHandleForHeapStart()
   //       );

   //     GetRenderer().RenderPass
   //       (
   //        dt, SHADER_DOMAIN_OPAQUE, true,
   //        [this](const StrongObjectBase& obj)
   //        {
   //          if (obj->GetID() == GetOwner().lock()->GetID())
   //          {
   //            return false;
   //          }

   //          return true;
   //        }, TODO, TODO, TODO
   //       );

   //     GetD3Device().GetCommandList(COMMAND_LIST_POST_RENDER)->ClearDepthStencilView
   //       (
   //        m_intensity_test_texs_[i].GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart(),
   //           D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
   //           1.f,
   //           0,
   //           0,
   //           nullptr
   //       );

   //     GetRenderPipeline().SetParam<int>(0, target_light_slot);
   //     GetRenderPipeline().SetParam<int>(false, custom_vp_slot);

   //     GetRenderPipeline().SetRenderTargetDeferred(COMMAND_LIST_POST_RENDER, prev_rtv);

   //     const auto& test_transition_back = CD3DX12_RESOURCE_BARRIER::Transition
   //       (
   //        m_intensity_test_texs_[i].GetRawResoruce(),
   //        D3D12_RESOURCE_STATE_RENDER_TARGET,
   //        D3D12_RESOURCE_STATE_COMMON
   //       );

   //     const auto& position_transition_back = CD3DX12_RESOURCE_BARRIER::Transition
   //       (
   //        m_intensity_position_texs_[i].GetRawResoruce(),
   //        D3D12_RESOURCE_STATE_RENDER_TARGET,
   //        D3D12_RESOURCE_STATE_COMMON
   //       );

   //     GetD3Device().GetCommandList(COMMAND_LIST_POST_RENDER)->ResourceBarrier(1, &test_transition_back);

   //     GetD3Device().GetCommandList(COMMAND_LIST_POST_RENDER)->ResourceBarrier(1, &position_transition_back);
   //   }

   //   GetRenderPipeline().FallbackPSO(COMMAND_LIST_POST_RENDER);
   //   m_sb_light_vp_.UnbindSRVGraphic(TODO);

   //   GetRenderPipeline().DefaultViewport(COMMAND_LIST_POST_RENDER);

   //   // By compute shader, For each pixel if it has light index, which is non-zero,
   //   // then this object shadow intersects with designated light.
   //   for (int i = 0; i < lights->size(); ++i)
   //   {
   //     const auto& cast = m_intersection_compute_->GetSharedPtr<ComputeShaders::IntersectionCompute>();

   //     cast->SetIntersectionTexture(m_intensity_test_texs_[i]);
   //     cast->SetPositionTexture(m_intensity_position_texs_[i]);
   //     cast->SetLightTable(&m_sb_light_table_);
   //     cast->SetTargetLight(i);
   //     cast->Dispatch(TODO, TODO);
   //   }

   //   std::vector<ComputeShaders::IntersectionCompute::LightTableSB> light_table;
   //   light_table.resize(g_max_lights);

   //   m_sb_light_table_.GetData(g_max_lights, light_table.data());

   //   for (int i = 0; i < lights->size(); ++i)
   //   {
   //     for (int j = 0; j < lights->size(); ++j)
   //     {
   //       if (light_table[i].lightTable[j].value > 0)
   //       {
   //         const auto wp_min = Vector3(light_table[i].min[j]) / light_table[i].min[j].w;
   //         const auto wp_max = Vector3(light_table[i].max[j]) / light_table[i].max[j].w;

   //         const auto average = Vector3::Lerp(wp_min, wp_max , 0.5f);

   //         GetDebugger().Draw(BoundingSphere(average, 0.1f), Colors::YellowGreen);

   //         BoundingBox bbox;
   //         BoundingBox::CreateFromPoints(
   //             bbox, 
   //             wp_min, 
   //             wp_max);

   //         m_shadow_bbox_.emplace(std::make_pair(i, j), bbox);
   //       }
   //     }
   //   }

   //   std::vector<ComputeShaders::IntersectionCompute::LightTableSB> empty_light_table;
   //   empty_light_table.resize(g_max_lights);

   //   m_sb_light_table_.SetData(g_max_lights, empty_light_table.data());
   //   m_sb_light_table_.BindSRVGraphic(TODO, TODO);
   // }
  }

  void ShadowIntersectionScript::OnCollisionEnter(const WeakCollider& other) {}

  void ShadowIntersectionScript::OnCollisionContinue(const WeakCollider& other) {}

  void ShadowIntersectionScript::OnCollisionExit(const WeakCollider& other) {}

  ShadowIntersectionScript::ShadowIntersectionScript()
    : m_viewport_() {}
}
