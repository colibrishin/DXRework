#include "pch.h"
#include "egShadowManager.hpp"

#include "Renderer.h"
#include "egAnimator.h"
#include "egBaseAnimation.h"
#include "egBoneAnimation.h"
#include "egObject.hpp"
#include "egResourceManager.hpp"
#include "egScene.hpp"
#include "egSceneManager.hpp"

#include "egCamera.h"
#include "egShadowTexture.h"
#include "egGlobal.h"
#include "egLight.h"
#include "egMaterial.h"
#include "egMesh.h"
#include "egModelRenderer.h"
#include "egProjectionFrustum.h"
#include "egShader.hpp"
#include "egShape.h"
#include "egTransform.h"

namespace Engine::Manager::Graphics
{
  void ShadowManager::Initialize()
  {
    m_shadow_shaders_ = Resources::Material::Create("ShadowMap", "");
    m_shadow_shaders_->SetResource<Resources::Shader>("cascade_shadow_stage1");

    m_sb_light_buffer_.Create(g_max_lights, nullptr, true);
    m_sb_light_vps_buffer_.Create(g_max_lights, nullptr, true);

    InitializeViewport();
    InitializeProcessor();
  }

  void ShadowManager::PreUpdate(const float& dt)
  {
    // Unbind the light information structured buffer from the shader.
    m_sb_light_buffer_.Unbind(SHADER_VERTEX);
    m_sb_light_buffer_.Unbind(SHADER_PIXEL);

    // And the light view and projection matrix buffer to re-evaluate.
    m_sb_light_vps_buffer_.Unbind(SHADER_PIXEL);

    // Remove the expired lights just in case.
    std::erase_if(m_lights_, [](const auto& kv) { return kv.second.expired(); });
  }

  void ShadowManager::Update(const float& dt)
  {
    constexpr size_t light_slot = 0;

    // Build light information structured buffer.
    std::vector<SBs::LightSB> light_buffer;

    for (const auto& light : m_lights_ | std::views::values)
    {
      if (const auto locked = light.lock())
      {
        const auto tr = locked->GetComponent<Components::Transform>().lock();

        const auto world = tr->GetWorldMatrix();

        light_buffer.emplace_back(world.Transpose(), locked->GetColor());
      }
    }

    // Notify the number of lights to the shader.
    GetRenderPipeline().SetParam<int>(m_lights_.size(), light_slot);

    // If there is no light, it does not need to be updated.
    if (light_buffer.empty()) { return; }

    m_sb_light_buffer_.SetData(light_buffer.size(), light_buffer.data());
  }

  void ShadowManager::PreRender(const float& dt)
  {
    constexpr size_t shadow_slot = 1;

    // # Pass 1 : depth only, building shadow map
    // Unbind the shadow map resource from the pixel shader to build the shadow map.
    GetRenderPipeline().UnbindResource(RESERVED_SHADOW_MAP, SHADER_PIXEL);
    
    // Clear all shadow map data.
    ClearShadowMaps();
    // Set the viewport to the size of the shadow map.
    GetRenderPipeline().SetViewport(m_viewport_);

    // bind the shadow map shaders.
    m_shadow_shaders_->PreRender(placeholder);
    m_shadow_shaders_->Render(placeholder);

    if (const auto scene = GetSceneManager().GetActiveScene().lock())
    {
      // Build light view and projection matrix in frustum.
      std::vector<SBs::LightVPSB> current_light_vp;

      for (const auto& ptr_light : m_lights_ | std::views::values)
      {
        if (const auto light = ptr_light.lock())
        {
          const auto tr = light->GetComponent<Components::Transform>().lock();

          // Get the light direction from the light's position.
          Vector3 light_dir;
          (tr->GetWorldPosition()).Normalize(light_dir);

          SBs::LightVPSB light_vp{};
          // Get the light's view and projection matrix in g_max_shadow_cascades parts.
          EvalShadowVP(light_dir, light_vp);
          current_light_vp.emplace_back(light_vp);
        }
      }

      // Also, if there is no light, it does not need to be updated.
      if (current_light_vp.empty()) { return; }

      m_sb_light_vps_buffer_.SetData(current_light_vp.size(), current_light_vp.data());
      m_sb_light_vps_buffer_.Bind(SHADER_GEOMETRY);

      UINT idx = 0;

      for (const auto& ptr_light : m_lights_ | std::views::values)
      {
        if (const auto light = ptr_light.lock())
        {
          // It only needs to render the depth of the object from the light's point of view.
          // Swap the depth stencil to the each light's shadow map.
          m_shadow_texs_[light->GetLocalID()].BindAs(D3D11_BIND_DEPTH_STENCIL, 0, 0, SHADER_UNKNOWN);
          m_shadow_texs_[light->GetLocalID()].Render(placeholder);

          // Notify the index of the shadow map to the shader.
          GetRenderPipeline().SetParam<int>(idx++, shadow_slot);
          // Render the depth of the object from the light's point of view.
          BuildShadowMap(dt);

          // Cleanup
          m_shadow_texs_[light->GetLocalID()].PostRender(placeholder);
        }
      }

      // Geometry shader's work is done.
      m_sb_light_vps_buffer_.Unbind(SHADER_GEOMETRY);
    }

    GetRenderPipeline().SetParam<int>(0, shadow_slot);

    // Unload the shadow map shaders.
    m_shadow_shaders_->PostRender(placeholder);

    // post-processing for serialization and binding
    std::vector<ID3D11ShaderResourceView*> current_shadow_maps;

    for (const auto& buffer : m_shadow_texs_ | std::views::values)
    {
      current_shadow_maps.emplace_back(buffer.GetSRV());
    }

    // Pass #2 : Render scene with shadow map
    // Reverts the viewport to the size of the screen, render target, and the depth stencil state.
    GetRenderPipeline().DefaultViewport();
    GetRenderPipeline().DefaultRenderTarget();
    GetRenderPipeline().DefaultDepthStencilState();

    // todo: refactoring.
    // Bind the shadow map resource previously rendered to the pixel shader.
    GetRenderPipeline().BindResources
      (RESERVED_SHADOW_MAP, SHADER_PIXEL, current_shadow_maps.data(), current_shadow_maps.size());
    // And bind the light view and projection matrix on to the constant buffer.
    m_sb_light_vps_buffer_.Bind(SHADER_PIXEL);

    // Bind the light information structured buffer that previously built.
    m_sb_light_buffer_.Bind(SHADER_PIXEL);
    m_sb_light_buffer_.Bind(SHADER_VERTEX);
  }

  void ShadowManager::Render(const float& dt) {}

  void ShadowManager::PostRender(const float& dt) {}

  void ShadowManager::FixedUpdate(const float& dt) {}

  void ShadowManager::PostUpdate(const float& dt) {}

  void ShadowManager::Reset()
  {
    m_lights_.clear();
    m_shadow_texs_.clear();

    for (auto& subfrusta : m_subfrusta_) { subfrusta = {}; }
  }

  void ShadowManager::BuildShadowMap(const float dt) const
  {
    GetRenderer().RenderPass
      (
       dt, false, true,
       [this](const StrongObject& obj)
       {
         if (obj->GetLayer() == LAYER_CAMERA || obj->GetLayer() == LAYER_UI || obj->GetLayer() == LAYER_ENVIRONMENT ||
             obj->GetLayer() == LAYER_LIGHT || obj->GetLayer() == LAYER_SKYBOX) { return false; }

         return true;
       }
      );
  }

  void ShadowManager::CreateSubfrusta(
    const Matrix& projection, float start,
    float         end, Subfrusta&   subfrusta
  ) const
  {
    BoundingFrustum frustum(projection);

    frustum.Near = start;
    frustum.Far  = end;

    static constexpr XMVECTORU32 vGrabY = {
      0x00000000, 0xFFFFFFFF, 0x00000000,
      0x00000000
    };
    static constexpr XMVECTORU32 vGrabX = {
      0xFFFFFFFF, 0x00000000, 0x00000000,
      0x00000000
    };

    const Vector4 rightTopV   = {frustum.RightSlope, frustum.TopSlope, 1.f, 1.f};
    const Vector4 leftBottomV = {
      frustum.LeftSlope, frustum.BottomSlope, 1.f,
      1.f
    };
    const Vector4 nearV = {frustum.Near, frustum.Near, frustum.Near, 1.f};
    const Vector4 farV  = {frustum.Far, frustum.Far, frustum.Far, 1.f};

    const Vector4 rightTopNear   = rightTopV * nearV;
    const Vector4 righTopFar     = rightTopV * farV;
    const Vector4 LeftBottomNear = leftBottomV * nearV;
    const Vector4 LeftBottomFar  = leftBottomV * farV;

    subfrusta.corners[0] = rightTopNear;
    subfrusta.corners[1] = XMVectorSelect(rightTopNear, LeftBottomNear, vGrabX);
    subfrusta.corners[2] = LeftBottomNear;
    subfrusta.corners[3] = XMVectorSelect(rightTopNear, LeftBottomNear, vGrabY);

    subfrusta.corners[4] = righTopFar;
    subfrusta.corners[5] = XMVectorSelect(righTopFar, LeftBottomFar, vGrabX);
    subfrusta.corners[6] = LeftBottomFar;
    subfrusta.corners[7] = XMVectorSelect(righTopFar, LeftBottomFar, vGrabY);
  }

  void ShadowManager::EvalShadowVP(const Vector3& light_dir, SBs::LightVPSB& buffer)
  {
    // https://cutecatgame.tistory.com/6

    if (const auto scene = GetSceneManager().GetActiveScene().lock())
    {
      if (const auto camera = scene->GetMainCamera().lock())
      {
        const float near_plane = g_screen_near;
        const float far_plane  = g_screen_far;

        const float cascadeEnds[]{near_plane, 10.f, 80.f, far_plane};

        // for cascade shadow mapping, total 3 parts are used.
        // (near, 6), (6, 18), (18, far)
        for (auto i = 0; i < g_max_shadow_cascades; ++i)
        {
          // frustum = near points 4 + far points 4
          CreateSubfrusta
            (
             camera->GetProjectionMatrix(), cascadeEnds[i],
             cascadeEnds[i + 1], m_subfrusta_[i]
            );

          const auto view_inv = camera->GetViewMatrix().Invert();

          // transform to world space
          Vector4 center{};
          for (auto& corner : m_subfrusta_[i].corners)
          {
            corner = Vector4::Transform(corner, view_inv);
            center += corner;
          }

          // Get center by averaging
          center /= 8.f;

          float radius = 0.f;
          for (const auto& corner : m_subfrusta_[i].corners)
          {
            float distance = Vector4::Distance(center, corner);
            radius         = std::max(radius, distance);
          }

          radius = std::ceil(radius * 16.f) / 16.f;

          auto          maxExtent      = Vector3{radius, radius, radius};
          Vector3       minExtent      = -maxExtent;
          const Vector3 cascadeExtents = maxExtent - minExtent;

          const auto pos = center + (light_dir * std::fabsf(minExtent.z));

          // DX11 uses row major matrix

          buffer.view[i] = XMMatrixTranspose
            (
             XMMatrixLookAtLH(pos, Vector3(center), Vector3::Up)
            );

          buffer.proj[i] =
            XMMatrixTranspose
            (
             XMMatrixOrthographicOffCenterLH
             (
              minExtent.x, maxExtent.x, minExtent.y,
              maxExtent.y, 0.f,
              cascadeExtents.z
             )
            );

          buffer.end_clip_spaces[i] =
            Vector4{0.f, 0.f, cascadeEnds[i + 1], 1.f};
          buffer.end_clip_spaces[i] =
            Vector4::Transform
            (
             buffer.end_clip_spaces[i],
             camera->GetProjectionMatrix()
            ); // use z axis
        }
      }
    }
  }

  void ShadowManager::RegisterLight(const WeakLight& light)
  {
    if (const auto locked = light.lock())
    {
      m_lights_[locked->GetLocalID()] = light;
      InitializeShadowBuffer(locked->GetLocalID());
    }
  }

  void ShadowManager::UnregisterLight(const WeakLight& light)
  {
    if (const auto locked = light.lock())
    {
      m_lights_.erase(locked->GetLocalID());
      m_shadow_texs_.erase(locked->GetLocalID());
    }
  }

  void ShadowManager::InitializeShadowBuffer(const LocalActorID id)
  {
    // This will bypass the resource manager for simplicity.
    m_shadow_texs_[id] = Resources::ShadowTexture();
    m_shadow_texs_[id].Load();
  }

  ShadowManager::~ShadowManager() { if (m_shadow_sampler_) { m_shadow_sampler_->Release(); } }

  void ShadowManager::InitializeProcessor()
  {
    D3D11_SAMPLER_DESC sampler_desc{};
    sampler_desc.Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_LESS;
    sampler_desc.BorderColor[0] = 1.f;
    sampler_desc.BorderColor[1] = 1.f;
    sampler_desc.BorderColor[2] = 1.f;
    sampler_desc.BorderColor[3] = 1.f;

    GetD3Device().CreateSampler(sampler_desc, m_shadow_sampler_.GetAddressOf());
    GetD3Device().GetContext()->PSSetSamplers(SAMPLER_SHADOW, 1, m_shadow_sampler_.GetAddressOf());
  }

  void ShadowManager::InitializeViewport()
  {
    m_viewport_.Width    = static_cast<float>(g_max_shadow_map_size);
    m_viewport_.Height   = static_cast<float>(g_max_shadow_map_size);
    m_viewport_.MinDepth = 0.f;
    m_viewport_.MaxDepth = 1.f;
    m_viewport_.TopLeftX = 0.f;
    m_viewport_.TopLeftY = 0.f;
  }

  void ShadowManager::ClearShadowMaps()
  {
    for (const auto& buffer : m_shadow_texs_ | std::views::values)
    {
      buffer.Clear();
    }
  }
} // namespace Engine::Manager::Graphics
