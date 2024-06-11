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
    m_shadow_shader_ = Resources::Shader::Get("cascade_shadow_stage1").lock();

    m_sb_light_buffer_ = boost::make_shared<StructuredBuffer<SBs::LightSB>>();
    m_sb_light_vps_buffer_ = boost::make_shared<StructuredBuffer<SBs::LightVPSB>>();

    const auto& cmd = GetD3Device().AcquireCommandPair(L"Shadow Manager Initialization").lock();

    cmd->SoftReset();

    m_sb_light_buffer_->Create(cmd->GetList(), g_max_lights, nullptr);
    m_sb_light_vps_buffer_->Create(cmd->GetList(), g_max_lights, nullptr);

    cmd->FlagReady();

    // Render target for shadow map mask.
    m_shadow_map_mask_ = Resources::Texture2D
      (
       "",
       {
         .Alignment = 0,
         .Width = g_max_shadow_map_size,
         .Height = g_max_shadow_map_size,
         .DepthOrArraySize = g_max_shadow_cascades,
         .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
         .Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
         .MipsLevel = 1,
         .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
         .SampleDesc = {1, 0},
        }
      );

    m_shadow_map_mask_.Load();
    m_shadow_map_mask_.Initialize();

    InitializeViewport();
    InitializeProcessor();

    GetRenderer().AppendAdditionalStructuredBuffer(m_sb_light_buffer_);
    GetRenderer().AppendAdditionalStructuredBuffer(m_sb_light_vps_buffer_);

    m_local_param_buffers_.resize(g_max_lights);
  }

  void ShadowManager::PreUpdate(const float& dt)
  {
    // Remove the expired lights just in case.
    std::erase_if(m_lights_, [](const auto& kv) { return kv.second.expired(); });

    m_local_param_buffers_.resize(m_lights_.size());

    for (auto& buffer : m_local_param_buffers_)
    {
      if (!buffer)
      {
        buffer = boost::make_shared<StructuredBuffer<SBs::LocalParamSB>>();
      }
    }

    for (const auto& heap : m_shadow_descriptor_heap_)
    {
      heap->Release();
    }

    m_shadow_descriptor_heap_.clear();
  }

  void ShadowManager::Update(const float& dt) {}

  void ShadowManager::GetLightVP(const boost::shared_ptr<Scene>& scene, std::vector<SBs::LightVPSB>& current_light_vp)
  {
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
        EvalShadowVP(scene->GetMainCamera(), light_dir, light_vp);
        current_light_vp.emplace_back(light_vp);
      }
    }
  }

  void ShadowManager::PreRender(const float& dt)
  {
    constexpr size_t shadow_slot = 1;
    constexpr size_t light_slot = 0;

    // # Pass 1 : depth only, building shadow map

    // Build light information structured buffer.
    std::vector<SBs::LightSB> light_buffer;

    for (const auto& light : m_lights_ | std::views::values)
    {
      if (const auto locked = light.lock())
      {
        const auto tr = locked->GetComponent<Components::Transform>().lock();

        const auto world = tr->GetWorldMatrix();

        light_buffer.emplace_back
          (
           world.Transpose(),
           locked->GetColor(),
           locked->GetType(),
           locked->GetRange()
          );
      }
    }

    // Notify the number of lights to the shader.
    GetRenderPipeline().SetParam<int>(static_cast<UINT>(m_lights_.size()), light_slot);

    // If there is no light, it does not need to be updated.
    if (light_buffer.empty())
    {
      return;
    }

    if (const auto scene = GetSceneManager().GetActiveScene().lock())
    {
      std::vector<SBs::LightVPSB> current_light_vp;
      GetLightVP(scene, current_light_vp);

      // Also, if there is no light, it does not need to be updated.
      if (current_light_vp.empty()) { return; }

      const auto& cmd = GetD3Device().AcquireCommandPair(L"Shadow Rendering").lock();

      cmd->SoftReset();

      ClearShadowMaps(cmd);

      m_sb_light_buffer_->SetData(cmd->GetList(), static_cast<UINT>(light_buffer.size()), light_buffer.data());
      m_sb_light_vps_buffer_->SetData(cmd->GetList(), static_cast<UINT>(current_light_vp.size()), current_light_vp.data());

      UINT idx = 0;

      UINT64 container_it = 0;

      m_shadow_instance_buffer_.resize(GetRenderer().GetInstanceCount() * m_lights_.size());

      for (const auto& ptr_light : m_lights_ | std::views::values)
      {
        if (const auto light = ptr_light.lock())
        {
          // Render the depth of the object from the light's point of view.
          container_it = BuildShadowMap(dt, container_it, cmd, light, idx++);
        }
      }

      cmd->FlagReady();
    }
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

  UINT64 ShadowManager::BuildShadowMap(const float dt, const UINT64 container_idx, const Weak<CommandPair>& w_cmd, const StrongLight & light, const UINT light_idx)
  {
    const auto& cmd = w_cmd.lock();

    // Notify the light index to the shader.
    SBs::LocalParamSB local_param{};
    local_param.SetParam(0, static_cast<int>(light_idx));
    m_local_param_buffers_[light_idx]->SetData(cmd->GetList(), 1, &local_param);

    return GetRenderer().RenderPass
      (
       dt, SHADER_DOMAIN_OPAQUE, true,
       cmd,
       container_idx, 
          m_shadow_descriptor_heap_,
       m_shadow_instance_buffer_, // todo: Instance data can be reused if it is re-rendering case?
          [this](const StrongObjectBase& obj)
       {
         if (obj->GetLayer() == LAYER_CAMERA || obj->GetLayer() == LAYER_UI || obj->GetLayer() == LAYER_ENVIRONMENT ||
             obj->GetLayer() == LAYER_LIGHT || obj->GetLayer() == LAYER_SKYBOX) { return false; }

         return true;
       }, [this, light, light_idx](const Weak<CommandPair>& wc, const DescriptorPtr& wh)
       {
         const auto& c = wc.lock();

         // Set viewport to the shadow map size.
         c->GetList()->RSSetViewports(1, &m_viewport_);
         c->GetList()->RSSetScissorRects(1, &m_scissor_rect_);
         // Bind the shadow map shader.
         c->GetList()->SetPipelineState(m_shadow_shader_->GetPipelineState());

         // It only needs to render the depth of the object from the light's point of view.
         // Swap the depth stencil to the each light's shadow map.
         const auto& dsv = m_shadow_texs_.at(light->GetLocalID());
         m_shadow_map_mask_.Bind(c, dsv);

         const auto& h = wh.lock();

         h->SetSampler(m_sampler_heap_->GetCPUDescriptorHandleForHeapStart(), SAMPLER_SHADOW);

         c->GetList()->IASetPrimitiveTopology(m_shadow_shader_->GetTopology());
       }, [this, light](const Weak<CommandPair>& c, const DescriptorPtr& h)
       {
         const auto& dsv = m_shadow_texs_.at(light->GetLocalID());

         m_shadow_map_mask_.Unbind(c, dsv);
       }, {m_sb_light_buffer_, m_sb_light_vps_buffer_, m_local_param_buffers_[light_idx]}
      );
  }

  void ShadowManager::CreateSubfrusta(
    const Matrix& projection, float start,
    float         end, Subfrusta&   subfrusta
  )
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

  void ShadowManager::EvalShadowVP(const WeakCamera& ptr_cam, const Vector3& light_dir, SBs::LightVPSB& buffer)
  {
    // https://cutecatgame.tistory.com/6
    if (const auto& camera = ptr_cam.lock())
    {
      const float near_plane = g_screen_near;
      const float far_plane  = g_screen_far;

      const float cascadeEnds[] {near_plane, 10.f, 80.f, far_plane};

      // for cascade shadow mapping, total 3 parts are used.
      // (near, 6), (6, 18), (18, far)
      for (auto i = 0; i < g_max_shadow_cascades; ++i)
      {
        Subfrusta subfrusta[g_max_shadow_cascades];

        // frustum = near points 4 + far points 4
        CreateSubfrusta
          (
           camera->GetProjectionMatrix(), cascadeEnds[i],
           cascadeEnds[i + 1], subfrusta[i]
          );

        const auto view_inv = camera->GetViewMatrix().Invert();

        // transform to world space
        Vector4 center{};
        for (auto& corner : subfrusta[i].corners)
        {
          corner = Vector4::Transform(corner, view_inv);
          center += corner;
        }

        // Get center by averaging
        center /= 8.f;

        float radius = 0.f;
        for (const auto& corner : subfrusta[i].corners)
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

  void ShadowManager::BindShadowMaps(const Weak<CommandPair>& w_cmd, const DescriptorPtr& w_heap) const
  {
    const auto& cmd = w_cmd.lock();
    const auto& heap = w_heap.lock();
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> current_shadow_maps;

    for (const auto& buffer : m_shadow_texs_ | std::views::values)
    {
      // todo: refactoring
      const auto& srv_transition = CD3DX12_RESOURCE_BARRIER::Transition
        (
         buffer.GetRawResoruce(),
         D3D12_RESOURCE_STATE_COMMON,
         D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
        );

      cmd->GetList()->ResourceBarrier(1, &srv_transition);

      current_shadow_maps.emplace_back(buffer.GetSRVDescriptor()->GetCPUDescriptorHandleForHeapStart());
    }

    // Bind the shadow map resource previously rendered to the pixel shader.
    heap->SetShaderResources
      (
       RESERVED_TEX_SHADOW_MAP,
       static_cast<UINT>(current_shadow_maps.size()),
       current_shadow_maps
      );
  }

  void ShadowManager::BindShadowSampler(const DescriptorPtr& w_heap) const
  {
    const auto& heap = w_heap.lock();
    heap->SetSampler(m_sampler_heap_->GetCPUDescriptorHandleForHeapStart(), SAMPLER_SHADOW);
  }

  void ShadowManager::UnbindShadowMaps(const Weak<CommandPair>& w_cmd) const
  {
    const auto& cmd = w_cmd.lock();

    for (const auto& buffer : m_shadow_texs_ | std::views::values)
    {
      buffer.Unbind(cmd, BIND_TYPE_SRV);
    }
  }

  Weak<StructuredBuffer<SBs::LightSB>> ShadowManager::GetLightBuffer() const
  {
    return m_sb_light_buffer_;
  }

  Weak<StructuredBuffer<SBs::LightVPSB>> ShadowManager::GetLightVPBuffer() const
  {
    return m_sb_light_vps_buffer_;
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

  ShadowManager::~ShadowManager() { }

  void ShadowManager::InitializeProcessor()
  {
    D3D12_SAMPLER_DESC sampler_desc{};
    sampler_desc.Filter         = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU       = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler_desc.AddressV       = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler_desc.AddressW       = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
    sampler_desc.BorderColor[0] = 1.f;
    sampler_desc.BorderColor[1] = 1.f;
    sampler_desc.BorderColor[2] = 1.f;
    sampler_desc.BorderColor[3] = 1.f;

    constexpr D3D12_DESCRIPTOR_HEAP_DESC sampler_heap_desc
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
      .NumDescriptors = 1,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
    };

    DX::ThrowIfFailed(GetD3Device().GetDevice()->CreateDescriptorHeap(&sampler_heap_desc, IID_PPV_ARGS(m_sampler_heap_.GetAddressOf())));

    GetD3Device().GetDevice()->CreateSampler
      (
       &sampler_desc, m_sampler_heap_->GetCPUDescriptorHandleForHeapStart()
      );
  }

  void ShadowManager::InitializeViewport()
  {
    m_viewport_.Width    = static_cast<float>(g_max_shadow_map_size);
    m_viewport_.Height   = static_cast<float>(g_max_shadow_map_size);
    m_viewport_.MinDepth = 0.f;
    m_viewport_.MaxDepth = 1.f;
    m_viewport_.TopLeftX = 0.f;
    m_viewport_.TopLeftY = 0.f;

    m_scissor_rect_.left   = 0;
    m_scissor_rect_.top    = 0;
    m_scissor_rect_.right  = g_max_shadow_map_size;
    m_scissor_rect_.bottom = g_max_shadow_map_size;
  }

  void ShadowManager::ClearShadowMaps(const Weak<CommandPair>& w_cmd)
  {
    const auto& cmd = w_cmd.lock();

    for (auto& tex : m_shadow_texs_ | std::views::values) { tex.Clear(cmd->GetList()); }

    constexpr float clear_color[4] = {0.f, 0.f, 0.f, 1.f};

    const auto& command_to_rtv = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_shadow_map_mask_.GetRawResoruce(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_RENDER_TARGET
      );

    const auto& rtv_to_common = CD3DX12_RESOURCE_BARRIER::Transition
      (
       m_shadow_map_mask_.GetRawResoruce(),
       D3D12_RESOURCE_STATE_RENDER_TARGET,
       D3D12_RESOURCE_STATE_COMMON
      );

    cmd->GetList()->ResourceBarrier(1, &command_to_rtv);

    cmd->GetList()->ClearRenderTargetView
      (
       m_shadow_map_mask_.GetRTVDescriptor()->GetCPUDescriptorHandleForHeapStart(),
       clear_color,
       0,
       nullptr
      );

    cmd->GetList()->ResourceBarrier(1, &rtv_to_common);
  }
} // namespace Engine::Manager::Graphics
