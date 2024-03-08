#include "pch.h"
#include "clShadowIntersectionScript.h"

#include "Renderer.h"
#include "egMaterial.h"
#include "egRenderPipeline.h"
#include "egSceneManager.hpp"
#include "egTransform.h"

namespace Client::Scripts
{
  void ShadowIntersectionScript::Initialize()
  {
	  Script::Initialize();

    if (const auto& material = Resources::Material::Get("ShadowMap").lock())
    {
      // borrow the shader from shadow manager.
      m_shadow_material_ = material;
    }

    for (auto& tex : m_shadow_texs_)
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

    // also borrow the sampler state from shadow manager.
  }

  void ShadowIntersectionScript::PreUpdate(const float& dt)
  {
	  for (auto& tex : m_shadow_texs_)
    {
      tex.Clear();
    }
  }

  void ShadowIntersectionScript::Update(const float& dt)
  {
    GetRenderPipeline().SetViewport(m_viewport_);

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
      m_sb_light_vp_.BindSRV(SHADER_GEOMETRY);

      UINT idx = 0;

      for (const auto& light : *lights)
      {
        m_shadow_texs_[idx].BindAs(D3D11_BIND_DEPTH_STENCIL, 0, 0, SHADER_UNKNOWN);
        m_shadow_texs_[idx].PreRender(0.f);
        m_shadow_texs_[idx].Render(0.f);

        GetRenderPipeline().SetParam<int>(idx, shadow_slot);

        GetRenderer().RenderPass
          (
           dt, SHADER_DOMAIN_OPAQUE, true,
           [this](const StrongObject& obj)
           {
             if (obj->GetID() != GetOwner().lock()->GetID())
             {
               return false;
             }

             return true;
           }
          );

        m_shadow_texs_[idx++].PostRender(0.f);
      }

      m_shadow_material_->PostRender(0.f);
      m_sb_light_vp_.UnbindSRV(SHADER_GEOMETRY);

      GetRenderPipeline().SetParam<int>(0, shadow_slot);

      GetRenderPipeline().DefaultViewport();
      GetRenderPipeline().DefaultRenderTarget();

      std::vector<ID3D11ShaderResourceView*> shadow_srv;
      shadow_srv.reserve(g_max_lights);

      for (const auto& tex : m_shadow_texs_)
      {
        shadow_srv.push_back(tex.GetSRV());
      }

      GetRenderPipeline().BindResources
        (
         RESERVED_SHADOW_MAP,
         SHADER_PIXEL,
         shadow_srv.data(),
         shadow_srv.size()
        );

      m_sb_light_vp_.BindSRV(SHADER_VERTEX);
      m_sb_light_vp_.BindSRV(SHADER_PIXEL);

      // TODO: mocking default render, check whether shadow is leaning onto the object where light intensity is above ambient color.

      // In light VP Render objects,
      // In pixel shader, while sampling shadow factor, do not use own shadow map
      // If shadow factor is above 0.f, and light intensity is above 0.f
      // then it intersects with light and shadow
      // Fill the pixel with light index (Do not use the sampler state, need raw value)

      // By compute shader, For each pixel if it has light index, which is non-zero,
      // then this object shadow intersects with designated light.
    }
  }

  void ShadowIntersectionScript::PostUpdate(const float& dt) {}

  void ShadowIntersectionScript::FixedUpdate(const float& dt) {}

  void ShadowIntersectionScript::PreRender(const float& dt) {}

  void ShadowIntersectionScript::Render(const float& dt) {}

  void ShadowIntersectionScript::PostRender(const float& dt) {}

  ShadowIntersectionScript::ShadowIntersectionScript()
    : m_viewport_() {}
}
