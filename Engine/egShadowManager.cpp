#include "pch.h"
#include "egShadowManager.hpp"

#include "egBaseAnimation.h"
#include "egAnimator.h"
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
#include "egTransform.h"
#include "egVertexShaderInternal.h"
#include "egShader.hpp"
#include "egShape.h"

namespace Engine::Manager::Graphics
{
    void ShadowManager::Initialize()
	{
    	m_shadow_shaders_ = Resources::Material::Create("ShadowMap", "");
        m_shadow_shaders_->SetResource<Resources::VertexShader>("vs_cascade_shadow_stage1");
        m_shadow_shaders_->SetResource<Resources::GeometryShader>("gs_cascade_shadow_stage1");
        m_shadow_shaders_->SetResource<Resources::PixelShader>("ps_cascade_shadow_stage1");

        InitializeViewport();
        InitializeProcessors();
    }

    void ShadowManager::PreUpdate(const float& dt)
    {
        // todo: cleanup non-lockable lights
    }

    void ShadowManager::Update(const float& dt)
    {
        for (const auto& light : m_lights_)
        {
            const UINT idx = static_cast<UINT>(std::distance(m_lights_.begin(), m_lights_.find(light)));

            if (const auto locked = light.lock())
            {
                const auto tr = locked->GetComponent<Components::Transform>().lock();

                const auto world = tr->GetWorldMatrix();

                m_light_buffer_.color[idx] = locked->GetColor();
                m_light_buffer_.world[idx] = world.Transpose();
            }
        }

        m_light_buffer_.light_count = static_cast<UINT>(m_lights_.size());
        GetRenderPipeline().SetLight(m_light_buffer_);
    }

    void ShadowManager::PreRender(const float& dt)
    {
        // # Pass 1 : depth only, building shadow map
        // Unbind the shadow map resource from the pixel shader to build the shadow map.
        GetRenderPipeline().UnbindResource(RESERVED_SHADOW_MAP, SHADER_PIXEL);

        // Clear all shadow map data.
        ClearShadowMaps();
        ClearShadowVP();
        // Set the viewport to the size of the shadow map.
        GetRenderPipeline().SetViewport(m_viewport_);

        // bind the shadow map shaders.
        m_shadow_shaders_->PreRender(placeholder);
        m_shadow_shaders_->Render(placeholder);

        if (const auto scene = GetSceneManager().GetActiveScene().lock())
        {
            for (const auto& ptr_light : m_lights_)
            {
                const UINT light_idx =
                        static_cast<UINT>(std::distance(m_lights_.begin(), m_lights_.find(ptr_light)));

                if (const auto light = ptr_light.lock())
                {
                    const auto tr = light->GetComponent<Components::Transform>().lock();

                    // It only needs to render the depth of the object from the light's point of view.
                    GetRenderPipeline().TargetDepthOnly(
                                                        m_dx_resource_shadow_vps_[ptr_light].depth_stencil_view.Get(),
                                                        m_shadow_map_depth_stencil_state_.Get());

                    Vector3 light_dir;
                    (tr->GetWorldPosition()).Normalize(light_dir);

                    // Get the light's view and projection matrix in g_max_shadow_cascades parts.
                    EvalShadowVP(
                                 light_dir, m_cb_shadow_vps_[light], light_idx,
                                 m_cb_shadow_vps_chunk_);

                    // Set the light's view and projection matrix to render the objects as the light sees them.
                    GetRenderPipeline().SetCascadeBuffer(m_cb_shadow_vps_[light]);
                    // Now render.
                    BuildShadowMap(*scene, dt);

                    // Save this light perspective depth buffer to shadow map.
                    m_cb_shadow_vps_chunk_.lights[light_idx] = m_cb_shadow_vps_[light].value;
                }
                else
                {
                    m_lights_.erase(ptr_light);
                }
            }
        }

        // Unload the shadow map shaders.
        m_shadow_shaders_->PostRender(placeholder);
        
        // post-processing for serialization and binding
        UINT light_idx = 0;

        for (const auto& buffer : m_dx_resource_shadow_vps_ | std::views::values)
        {
            m_current_shadow_maps_[light_idx] = buffer.shader_resource_view.Get();
            light_idx++;
        }

        // Pass #2 : Render scene with shadow map
        // Reverts the viewport to the size of the screen, render target, and the depth stencil state.
        GetRenderPipeline().DefaultViewport();
        GetRenderPipeline().DefaultRenderTarget();
        GetRenderPipeline().DefaultDepthStencilState();

        // Bind the shadow map resource previously rendered to the pixel shader.
        GetRenderPipeline().BindResources(
                                          RESERVED_SHADOW_MAP, SHADER_PIXEL,
                                          m_current_shadow_maps_,
                                          light_idx);
        // And bind the light view and projection matrix on to the constant buffer.
        GetRenderPipeline().SetShadowVP(m_cb_shadow_vps_chunk_);
    }

    void ShadowManager::Render(const float& dt) {}

    void ShadowManager::PostRender(const float& dt) {}

    void ShadowManager::FixedUpdate(const float& dt) {}

    void ShadowManager::PostUpdate(const float& dt) {}

    void ShadowManager::Reset()
    {
        // it will not clear the graphic shadow buffer, because it can just overwrite
        // the buffer and reuse it.
        m_lights_.clear();
        m_cb_shadow_vps_.clear();

        for (auto& subfrusta : m_subfrusta_)
        {
            subfrusta = {};
        }
    }

    void ShadowManager::ClearShadowVP()
    {
        for (auto& buffer : m_cb_shadow_vps_chunk_.lights)
        {
            for (auto& view : buffer.view)
            {
                view = Matrix::Identity;
                view = view.Transpose();
            }

            for (auto& proj : buffer.proj)
            {
                proj = Matrix::Identity;
                proj = proj.Transpose();
            }

            for (auto& end_clip_space : buffer.end_clip_spaces)
            {
                end_clip_space = Vector4{0.f, 0.f, 0.f, 0.f};
            }
        }
    }

    void ShadowManager::BuildShadowMap(Scene& scene, const float dt) const
    {
        for (const auto& [type, layer] : scene)
        {
            if (type == LAYER_LIGHT || type == LAYER_UI || type == LAYER_CAMERA ||
                type == LAYER_SKYBOX || type == LAYER_ENVIRONMENT)
                continue;

            for (const auto& object : *layer)
            {
                const auto tr = object->GetComponent<Components::Transform>().lock();
                const auto mr = object->GetComponent<Components::ModelRenderer>().lock();
                const auto ptr_atr = object->GetComponent<Components::Animator>();

                if (tr && mr)
                {
                    Components::Transform::Bind(*tr);

                    const auto& ptr_model = mr->GetModel();
                    const auto& ptr_mtr = mr->GetMaterial();
                    if (ptr_model.expired()) continue;

                    const auto model = ptr_model.lock();
                    const auto mtr = ptr_mtr.lock();
                    float current_frame = dt;

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
                }
            }
        }
    }

    void ShadowManager::CreateSubfrusta(
        const Matrix& projection, float start,
        float         end, Subfrusta&   subfrusta) const
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

    void ShadowManager::EvalShadowVP(
        const Vector3&                 light_dir,
        CBs::ShadowVPCB&      buffer,
        const UINT                     light_index,
        CBs::ShadowVPChunkCB& chunk)
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
                    CreateSubfrusta(
                                    camera->GetProjectionMatrix(), cascadeEnds[i],
                                    cascadeEnds[i + 1], m_subfrusta_[i]);

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

                    buffer.value.view[i] = XMMatrixTranspose(
                                                              XMMatrixLookAtLH(pos, Vector3(center), Vector3::Up));

                    buffer.value.proj[i] =
                            XMMatrixTranspose(
                                              XMMatrixOrthographicOffCenterLH(
                                                                              minExtent.x, maxExtent.x, minExtent.y,
                                                                              maxExtent.y, 0.f,
                                                                              cascadeExtents.z));

                    buffer.value.end_clip_spaces[i] =
                            Vector4{0.f, 0.f, cascadeEnds[i + 1], 1.f};
                    buffer.value.end_clip_spaces[i] =
                            Vector4::Transform(
                                               buffer.value.end_clip_spaces[i],
                                               camera->GetProjectionMatrix()); // use z axis
                }
            }
        }
    }

    void ShadowManager::RegisterLight(const WeakLight& light)
    {
        m_lights_.insert(light);

        InitializeShadowBuffer(m_dx_resource_shadow_vps_[light]);
        m_cb_shadow_vps_[light] = {};
    }

    void ShadowManager::UnregisterLight(const WeakLight& light)
    {
        const UINT idx = static_cast<UINT>(std::distance(m_lights_.begin(), m_lights_.find(light)));

        m_lights_.erase(light);
        m_dx_resource_shadow_vps_.erase(light);
        m_cb_shadow_vps_.erase(light);
        m_light_buffer_.color[idx] = Colors::White;
        m_light_buffer_.world[idx] = Matrix::Identity;

        m_dx_resource_shadow_vps_[light] = {};
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

        GetD3Device().CreateDepthStencil(
                                         desc, dsv_desc, buffer.texture.ReleaseAndGetAddressOf(),
                                         buffer.depth_stencil_view.ReleaseAndGetAddressOf());

        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
        srv_desc.Format                         = DXGI_FORMAT_R32_FLOAT;
        srv_desc.ViewDimension                  = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        srv_desc.Texture2DArray.ArraySize       = g_max_shadow_cascades;
        srv_desc.Texture2DArray.FirstArraySlice = 0;
        srv_desc.Texture2DArray.MipLevels       = 1;
        srv_desc.Texture2DArray.MostDetailedMip = 0;

        GetD3Device().CreateShaderResourceView(
                                                buffer.texture.Get(), srv_desc,
                                                buffer.shader_resource_view.ReleaseAndGetAddressOf());
    }

    ShadowManager::~ShadowManager()
    {
        if (m_shadow_map_depth_stencil_state_)
        {
            m_shadow_map_depth_stencil_state_->Release();
        }

        if (m_shadow_map_sampler_state_)
        {
            m_shadow_map_sampler_state_->Release();
        }
    }

    void ShadowManager::InitializeProcessors()
    {
        D3D11_DEPTH_STENCIL_DESC ds_desc{};

        ds_desc.DepthEnable                  = true;
        ds_desc.DepthWriteMask               = D3D11_DEPTH_WRITE_MASK_ALL;
        ds_desc.DepthFunc                    = D3D11_COMPARISON_LESS;
        ds_desc.StencilEnable                = true;
        ds_desc.StencilReadMask              = 0xFF;
        ds_desc.StencilWriteMask             = 0xFF;
        ds_desc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
        ds_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
        ds_desc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
        ds_desc.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;
        ds_desc.BackFace.StencilFailOp       = D3D11_STENCIL_OP_KEEP;
        ds_desc.BackFace.StencilDepthFailOp  = D3D11_STENCIL_OP_DECR;
        ds_desc.BackFace.StencilPassOp       = D3D11_STENCIL_OP_KEEP;
        ds_desc.BackFace.StencilFunc         = D3D11_COMPARISON_ALWAYS;

        GetD3Device().CreateDepthStencilState(
                                              ds_desc,
                                              m_shadow_map_depth_stencil_state_.
                                              GetAddressOf());

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

        GetD3Device().CreateSampler(sampler_desc, m_shadow_map_sampler_state_.GetAddressOf());
        GetRenderPipeline().BindSampler(m_shadow_map_sampler_state_.Get());
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
            GetD3Device().GetContext()->ClearDepthStencilView(
                                                              buffer.depth_stencil_view.Get(),
                                                              D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
        }
    }
} // namespace Engine::Manager::Graphics
