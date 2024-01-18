#include "pch.h"
#include "egShadowManager.hpp"

#include "egAnimator.h"
#include "egBaseAnimation.h"
#include "egBoneAnimation.h"
#include "egObject.hpp"
#include "egResourceManager.hpp"
#include "egScene.hpp"
#include "egSceneManager.hpp"

#include "egCamera.h"
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
    auto gcb        = GetRenderPipeline().GetGlobalStateBuffer();
    gcb.light_count = light_buffer.size();
    GetRenderPipeline().SetGlobalStateBuffer(gcb);

    // If there is no light, it does not need to be updated.
    if (light_buffer.empty()) { return; }

    m_sb_light_buffer_.SetData(light_buffer.size(), light_buffer.data());
  }

  void ShadowManager::PreRender(const float& dt)
  {
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
          GetRenderPipeline().TargetDepthOnly
            (
             m_dx_resource_shadow_vps_[light->GetLocalID()].depth_stencil_view.Get()
            );

          // Notify the index of the shadow map to the shader.
          auto gcb          = GetRenderPipeline().GetGlobalStateBuffer();
          gcb.target_shadow = idx++;
          GetRenderPipeline().SetGlobalStateBuffer(gcb);

          // Render the depth of the object from the light's point of view.
          BuildShadowMap(*scene, dt);
        }
      }

      // Geometry shader's work is done.
      m_sb_light_vps_buffer_.Unbind(SHADER_GEOMETRY);
    }

    // Unload the shadow map shaders.
    m_shadow_shaders_->PostRender(placeholder);

    // post-processing for serialization and binding
    std::vector<ID3D11ShaderResourceView*> current_shadow_maps;

    for (const auto& buffer : m_dx_resource_shadow_vps_ | std::views::values)
    {
      current_shadow_maps.emplace_back(buffer.shader_resource_view.Get());
    }

    // Pass #2 : Render scene with shadow map
    // Reverts the viewport to the size of the screen, render target, and the depth stencil state.
    GetRenderPipeline().DefaultViewport();
    GetRenderPipeline().DefaultRenderTarget();
    GetRenderPipeline().DefaultDepthStencilState();

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
    m_dx_resource_shadow_vps_.clear();

    for (auto& subfrusta : m_subfrusta_) { subfrusta = {}; }
  }

  void ShadowManager::BuildShadowMap(Scene& scene, const float dt) const
  {
    for (int i = 0; i < LAYER_MAX; ++i)
    {
      if (i == LAYER_LIGHT || i == LAYER_UI || i == LAYER_CAMERA ||
          i == LAYER_SKYBOX || i == LAYER_ENVIRONMENT) { continue; }

      for (const auto& object : *scene[i])
      {
        const auto tr      = object->GetComponent<Components::Transform>().lock();
        const auto mr      = object->GetComponent<Components::ModelRenderer>().lock();
        const auto ptr_atr = object->GetComponent<Components::Animator>();

        if (tr && mr)
        {
          Components::Transform::Bind(*tr);

          const auto& ptr_model = mr->GetModel();
          const auto& ptr_mtr   = mr->GetMaterial();
          if (ptr_model.expired()) { continue; }

          const auto model         = ptr_model.lock();
          const auto mtr           = ptr_mtr.lock();
          float      current_frame = dt;

          if (const auto atr = ptr_atr.lock())
          {
            if (const auto anim = mtr->GetResource<Resources::BoneAnimation>(atr->GetAnimation()).lock())
            {
              current_frame = atr->GetFrame();
              anim->PreRender(current_frame);
              anim->Render(current_frame);
            }
          }

          model->PreRender(current_frame);
          model->Render(current_frame);
          model->PostRender(current_frame);

          if (const auto atr = ptr_atr.lock())
          {
            if (const auto anim = mtr->GetResource<Resources::BoneAnimation>(atr->GetAnimation()).lock())
            {
              anim->PostRender(current_frame);
            }
          }

          Components::Transform::Unbind();
        }
      }
    }
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

          // DX11 uses column major matrix

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
      InitializeShadowBuffer(m_dx_resource_shadow_vps_[locked->GetLocalID()]);
    }
  }

  void ShadowManager::UnregisterLight(const WeakLight& light)
  {
    if (const auto locked = light.lock())
    {
      m_lights_.erase(locked->GetLocalID());
      m_dx_resource_shadow_vps_.erase(locked->GetLocalID());
    }
  }

  void ShadowManager::InitializeShadowBuffer(DXPacked::ShadowVPResource& buffer)
  {
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width              = g_max_shadow_map_size;
    desc.Height             = g_max_shadow_map_size;
    desc.MipLevels          = 1;
    desc.ArraySize          = g_max_shadow_cascades;
    desc.Format             = DXGI_FORMAT_R32_TYPELESS;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage              = D3D11_USAGE_DEFAULT;
    desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
    desc.CPUAccessFlags     = 0;
    desc.MiscFlags          = 0;

    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
    dsv_desc.Format                         = DXGI_FORMAT_D32_FLOAT;
    dsv_desc.ViewDimension                  = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
    dsv_desc.Texture2DArray.ArraySize       = g_max_shadow_cascades;
    dsv_desc.Texture2DArray.MipSlice        = 0;
    dsv_desc.Texture2DArray.FirstArraySlice = 0;
    dsv_desc.Flags                          = 0;

    GetD3Device().CreateDepthStencil
      (
       desc, dsv_desc, buffer.texture.ReleaseAndGetAddressOf(),
       buffer.depth_stencil_view.ReleaseAndGetAddressOf()
      );

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    srv_desc.Format                         = DXGI_FORMAT_R32_FLOAT;
    srv_desc.ViewDimension                  = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srv_desc.Texture2DArray.ArraySize       = g_max_shadow_cascades;
    srv_desc.Texture2DArray.FirstArraySlice = 0;
    srv_desc.Texture2DArray.MipLevels       = 1;
    srv_desc.Texture2DArray.MostDetailedMip = 0;

    GetD3Device().CreateShaderResourceView
      (
       buffer.texture.Get(), srv_desc,
       buffer.shader_resource_view.ReleaseAndGetAddressOf()
      );
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
    for (const auto& buffer : m_dx_resource_shadow_vps_ | std::views::values)
    {
      GetD3Device().GetContext()->ClearDepthStencilView
        (
         buffer.depth_stencil_view.Get(),
         D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0
        );
    }
  }
} // namespace Engine::Manager::Graphics
