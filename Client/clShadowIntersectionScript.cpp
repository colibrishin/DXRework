#include "pch.h"
#include "clShadowIntersectionScript.h"

#include <Renderer.h>
#include <egMaterial.h>
#include <egRenderPipeline.h>
#include <egSceneManager.hpp>
#include <egTransform.h>
#include <egShader.hpp>

#include "egCamera.h"
#include "egMouseManager.h"

namespace Client::Scripts
{
  SCRIPT_CLONE_IMPL(ShadowIntersectionScript)

  void ShadowIntersectionScript::Initialize()
  {
	  Script::Initialize();

    if (const auto& material = Engine::Resources::Material::Get("ShadowMap").lock())
    {
      // borrow the shader from shadow manager.
      m_shadow_material_ = material;
    }

    if (const auto& mtr = Engine::Resources::Material::Get("IntensityTest").lock())
    {
      m_intensity_test_material_ = mtr;
    }
    else
    {
      m_intensity_test_material_ = Engine::Resources::Material::Create("IntensityTest", "");
      m_intensity_test_material_->SetResource<Engine::Resources::Shader>("intensity_test");
    }

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

    m_sb_light_vp_.Create(g_max_lights, nullptr, true);

    m_viewport_.Width    = g_max_shadow_map_size;
    m_viewport_.Height   = g_max_shadow_map_size;
    m_viewport_.MinDepth = 0.0f;
    m_viewport_.MaxDepth = 1.0f;
    m_viewport_.TopLeftX = 0.0f;
    m_viewport_.TopLeftY = 0.0f;

    m_sb_light_table_.Create(g_max_lights, nullptr, true);

    // also borrow the sampler state from shadow manager.
  }

  void ShadowIntersectionScript::PreUpdate(const float& dt)
  {
    constexpr float clear_color[4] = { 0.f, 0.f, 0.f, 0.f };

	  for (auto& tex : m_shadow_texs_)
    {
      tex.Clear();
    }

    for (auto& tex : m_intensity_position_texs_)
    {
      GetD3Device().GetDirectCommandList()->ClearRenderTargetView
      (
          tex.GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart(),
          clear_color,
          0,
          nullptr
      );
    }

    for (auto& tex : m_intensity_test_texs_)
    {
      GetD3Device().GetDirectCommandList()->ClearRenderTargetView
      (
          tex.GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart(),
          clear_color,
          0,
          nullptr
      );
    }

    for (auto& tex : m_shadow_mask_texs_)
    {
      GetD3Device().GetDirectCommandList()->ClearRenderTargetView
      (
          tex.GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart(),
          clear_color,
          0,
          nullptr
      );
    }
  }

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
    GetRenderPipeline().SetViewportDeferred(m_viewport_);

    std::vector<Graphics::SBs::LightVPSB> light_vps;
    light_vps.reserve(g_max_lights);

    m_shadow_material_->PreRender(0.f);
    m_shadow_material_->Render(0.f);

    // Build shadow map of this object for each light.
    if (const auto& scene = GetOwner().lock()->GetScene().lock())
    {
      constexpr size_t shadow_slot = 1;
      const auto       lights      = (*scene)[LAYER_LIGHT];

      for (const auto& light : *lights)
      {
        const auto& tr = light->GetComponent<Components::Transform>().lock();

        Vector3 light_dir;
        (tr->GetWorldPosition()).Normalize(light_dir);

        Graphics::SBs::LightVPSB light_vp{};
        GetShadowManager().EvalShadowVP(scene->GetMainCamera(), light_dir, light_vp);

        light_vps.push_back(light_vp);
      }

      m_sb_light_vp_.SetData(light_vps.size(), light_vps.data());
      m_sb_light_vp_.BindSRVGraphicDeferred();

      UINT idx = 0;

      // Draw shadow map except the object itself.
      for (const auto& light : *lights)
      {
        m_shadow_texs_[idx].BindAs(BIND_TYPE_RTV, 0, 0);
        m_shadow_texs_[idx].PreRender(0.f);
        m_shadow_texs_[idx].Render(0.f);

        GetRenderPipeline().SetParam<int>(idx, shadow_slot);

        GetRenderer().RenderPass
          (
           dt, SHADER_DOMAIN_OPAQUE, true,
           [this](const StrongObjectBase& obj)
           {
             if (obj->GetID() == GetOwner().lock()->GetID())
             {
               return false;
             }

             return true;
           }
          );

        m_shadow_texs_[idx++].PostRender(0.f);
      }

      idx = 0;

      // Render object only for picking up the exact shadow position.
      for (const auto& light : *lights)
      {
        ID3D11RenderTargetView* previous_rtv = nullptr;
        ID3D11DepthStencilView* previous_dsv = nullptr;

        const auto& prev_rtv = GetRenderPipeline().SetRenderTargetDeferred
          (
           m_shadow_mask_texs_[idx].GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart(),
           m_shadow_texs_[idx].GetDSVDescriptor()->GetCPUDescriptorHandleForHeapStart()
          );

        GetRenderPipeline().SetParam<int>(idx, shadow_slot);

        GetRenderer().RenderPass
          (
           dt, SHADER_DOMAIN_OPAQUE, true,
           [this](const StrongObjectBase& obj)
           {
             if (obj->GetID() != GetOwner().lock()->GetID())
             {
               return false;
             }

             return true;
           }
          );

        GetRenderPipeline().SetRenderTargetDeferred(prev_rtv);

        idx++;
      }

      m_shadow_material_->PostRender(0.f);
      m_sb_light_vp_.BindSRVGraphicDeferred();

      GetRenderPipeline().SetParam<int>(0, shadow_slot);

      GetRenderPipeline().DefaultViewport();
      GetRenderPipeline().DefaultRenderTarget();

      std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> shadow_srv;
      shadow_srv.reserve(g_max_lights);

      for (const auto& tex : m_shadow_texs_)
      {
        shadow_srv.push_back(tex.GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart());
      }

      GetRenderPipeline().SetShaderResources
        (
         RESERVED_SHADOW_MAP,
         shadow_srv.size(),
         shadow_srv
        );

      m_sb_light_vp_.BindSRVGraphicDeferred();
      m_sb_light_vp_.BindSRVGraphicDeferred();

      m_intensity_test_material_->PreRender(0.f);
      m_intensity_test_material_->Render(0.f);

      constexpr size_t target_light_slot = 2;
      constexpr size_t custom_vp_slot    = 3;

      constexpr size_t custom_view_slot = 1;
      constexpr size_t custom_proj_slot = 2;

      // In light VP Render objects,
      // In pixel shader, while sampling shadow factor, do not use own shadow map
      // If shadow factor is above 0.f, and light intensity is above 0.f
      // then it intersects with light and shadow
      // Fill the pixel with light index (Do not use the sampler state, need raw value)

      GetRenderPipeline().SetViewportDeferred(m_viewport_);

      int z_clip = 0;

      if (const auto camera = scene->GetMainCamera().lock())
      {
        const float z = camera->GetComponent<Components::Transform>().lock()->GetWorldPosition().z;

        for (int i = 0; i < 3; ++i)
        {
          if (light_vps[0].end_clip_spaces[i].z > z )
          {
            z_clip = i;
            break;
          }
        }
      }

      std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> srv_ptr;

      for (auto& tex : m_shadow_mask_texs_)
      {
        srv_ptr.push_back(tex.GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart());
      }

      GetRenderPipeline().SetShaderResources
        (
         BIND_SLOT_TEXARR,
         static_cast<UINT>(srv_ptr.size()),
         srv_ptr
        );

      for (int i = 0; i < lights->size(); ++i)
      {
        GetRenderPipeline().SetParam<int>(i, target_light_slot);
        GetRenderPipeline().SetParam<int>(true, custom_vp_slot);

        GetRenderPipeline().SetParam<Matrix>(light_vps[i].view[z_clip], custom_view_slot);
        GetRenderPipeline().SetParam<Matrix>(light_vps[i].proj[z_clip], custom_proj_slot);

        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtv_ptr;
        rtv_ptr.push_back(m_intensity_test_texs_[i].GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart());
        rtv_ptr.push_back(m_intensity_position_texs_[i].GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart());

        const auto& test_transition = CD3DX12_RESOURCE_BARRIER::Transition
          (
           m_intensity_test_texs_[i].GetRawResoruce(),
           D3D12_RESOURCE_STATE_COMMON,
           D3D12_RESOURCE_STATE_RENDER_TARGET
          );

        const auto& position_transition = CD3DX12_RESOURCE_BARRIER::Transition
          (
           m_intensity_position_texs_[i].GetRawResoruce(),
           D3D12_RESOURCE_STATE_COMMON,
           D3D12_RESOURCE_STATE_RENDER_TARGET
          );

        GetD3Device().GetDirectCommandList()->ResourceBarrier(1, &test_transition);
        GetD3Device().GetDirectCommandList()->ResourceBarrier(1, &position_transition);

        const auto& prev_rtv = GetRenderPipeline().SetRenderTargetDeferred
          (
           rtv_ptr.size(),
           rtv_ptr.data(),
           m_tmp_shadow_depth_->GetDSVDescriptor()->GetCPUDescriptorHandleForHeapStart()
          );

        GetRenderer().RenderPass
          (
           dt, SHADER_DOMAIN_OPAQUE, true,
           [this](const StrongObjectBase& obj)
           {
             if (obj->GetID() == GetOwner().lock()->GetID())
             {
               return false;
             }

             return true;
           }
          );

        GetD3Device().GetDirectCommandList()->ClearDepthStencilView
          (
           m_intensity_test_texs_[i].GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart(),
              D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
              1.f,
              0,
              0,
              nullptr
          );

        GetRenderPipeline().SetParam<int>(0, target_light_slot);
        GetRenderPipeline().SetParam<int>(false, custom_vp_slot);

        GetRenderPipeline().SetRenderTargetDeferred(prev_rtv);

        const auto& test_transition_back = CD3DX12_RESOURCE_BARRIER::Transition
          (
           m_intensity_test_texs_[i].GetRawResoruce(),
           D3D12_RESOURCE_STATE_RENDER_TARGET,
           D3D12_RESOURCE_STATE_COMMON
          );

        const auto& position_transition_back = CD3DX12_RESOURCE_BARRIER::Transition
          (
           m_intensity_position_texs_[i].GetRawResoruce(),
           D3D12_RESOURCE_STATE_RENDER_TARGET,
           D3D12_RESOURCE_STATE_COMMON
          );

        GetD3Device().GetDirectCommandList()->ResourceBarrier(1, &test_transition_back);

        GetD3Device().GetDirectCommandList()->ResourceBarrier(1, &position_transition_back);
      }

      m_intensity_test_material_->PostRender(0.f);
      m_sb_light_vp_.UnbindSRVGraphicDeferred();

      GetRenderPipeline().DefaultViewport();

      // By compute shader, For each pixel if it has light index, which is non-zero,
      // then this object shadow intersects with designated light.
      for (int i = 0; i < lights->size(); ++i)
      {
        const auto& cast = m_intersection_compute_->GetSharedPtr<ComputeShaders::IntersectionCompute>();

        cast->SetIntersectionTexture(m_intensity_test_texs_[i]);
        cast->SetPositionTexture(m_intensity_position_texs_[i]);
        cast->SetLightTable(&m_sb_light_table_);
        cast->SetTargetLight(i);

        cast->Dispatch();
      }

      std::vector<ComputeShaders::IntersectionCompute::LightTableSB> light_table;
      light_table.resize(g_max_lights);

      m_sb_light_table_.GetData(g_max_lights, light_table.data());

      for (int i = 0; i < lights->size(); ++i)
      {
        for (int j = 0; j < lights->size(); ++j)
        {
          if (light_table[i].lightTable[j].value > 0)
          {
            const auto wp_min = Vector3(light_table[i].min[j]) / light_table[i].min[j].w;
            const auto wp_max = Vector3(light_table[i].max[j]) / light_table[i].max[j].w;

            const auto average = Vector3::Lerp(wp_min, wp_max , 0.5f);

            GetDebugger().Draw(BoundingSphere(average, 0.1f), Colors::YellowGreen);

            BoundingBox bbox;
            BoundingBox::CreateFromPoints(
                bbox, 
                wp_min, 
                wp_max);

            m_shadow_bbox_.emplace(std::make_pair(i, j), bbox);
          }
        }
      }

      std::vector<ComputeShaders::IntersectionCompute::LightTableSB> empty_light_table;
      empty_light_table.resize(g_max_lights);

      m_sb_light_table_.SetData(g_max_lights, empty_light_table.data());
    }
  }

  void ShadowIntersectionScript::OnCollisionEnter(const WeakCollider& other) {}

  void ShadowIntersectionScript::OnCollisionContinue(const WeakCollider& other) {}

  void ShadowIntersectionScript::OnCollisionExit(const WeakCollider& other) {}

  ShadowIntersectionScript::ShadowIntersectionScript()
    : m_viewport_() {}
}
