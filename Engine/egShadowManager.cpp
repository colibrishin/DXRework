#include "pch.hpp"
#include "egShadowManager.hpp"

#include "egObject.hpp"
#include "egResourceManager.hpp"
#include "egSceneManager.hpp"
#include "egScene.hpp"

#include "egTransform.hpp"
#include "egMesh.hpp"
#include "egCamera.hpp"
#include "egLight.hpp"
#include "egProjectionFrustum.hpp"

namespace Engine::Manager::Graphics
{
	void ShadowManager::Initialize()
	{
		m_vs_stage1 = GetResourceManager().GetResource<Graphic::VertexShader>("vs_cascade_shadow_stage1").lock();
		m_gs_stage1 = GetResourceManager().GetResource<Graphic::GeometryShader>("gs_cascade_shadow_stage1").lock();
		m_ps_stage1 = GetResourceManager().GetResource<Graphic::PixelShader>("ps_cascade_shadow_stage1").lock();

	}

	void ShadowManager::PreUpdate(const float& dt)
	{
		// todo: cleanup non-lockable lights
	}

	void ShadowManager::Update(const float& dt)
	{
		for (const auto& light : m_lights_)
		{
			const int idx = std::distance(m_lights_.begin(), m_lights_.find(light));

			if (const auto locked = light.lock())
			{
				const auto tr = locked->GetComponent<Engine::Component::Transform>().lock();

				const auto world = tr->GetWorldMatrix();

				GetRenderPipeline().SetLight(idx, world, locked->GetColor());
			}
		}

		GetRenderPipeline().BindLightBuffer(m_lights_.size());
	}

	void ShadowManager::BindShadowMapChunk()
	{
		ID3D11ShaderResourceView* shadow_maps[g_max_lights];
		size_t idx = 0;

		for (const auto& buffer : m_graphic_shadow_buffer_ | std::views::values)
		{
			shadow_maps[idx] = buffer.shader_resource_view.Get();
			idx++;
		}

		GetRenderPipeline().BindShadowMap(idx, shadow_maps);
	}

	void ShadowManager::ClearShadowBufferChunk()
	{
		for (auto& buffer : m_cascade_shadow_buffer_chunk_.lights)
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

	void ShadowManager::PreRender(const float& dt)
	{
		// # Pass 1 : depth only, building shadow map

		GetRenderPipeline().UseShadowMapViewport();

		m_vs_stage1->Render(placeholder);
		m_gs_stage1->Render(placeholder);
		m_ps_stage1->Render(placeholder);

		const auto scene = GetSceneManager().GetActiveScene().lock();

		if (!scene)
		{
			return;
		}

		for (const auto& known_light : m_lights_)
		{
			const auto idx = std::distance(m_lights_.begin(), m_lights_.find(known_light));

			if (const auto locked = known_light.lock())
			{
				const auto tr = locked->GetComponent<Engine::Component::Transform>().lock();

				GetRenderPipeline().TargetShadowMap(m_graphic_shadow_buffer_[known_light]);

				Vector3 light_dir;
				(tr->GetPosition()).Normalize(light_dir);

				EvalCascadeVP(light_dir, m_cascade_vp_buffer_[locked], idx, m_cascade_shadow_buffer_chunk_);
				GetRenderPipeline().SetCascadeBuffer(m_cascade_vp_buffer_[locked]);
				BuildShadowMap(*scene);
				m_cascade_shadow_buffer_chunk_.lights[idx] = m_cascade_vp_buffer_[locked].shadow;
			}
			else
			{
				m_lights_.erase(known_light);
			}
		}
		
		GetRenderPipeline().ResetShaders();
		GetRenderPipeline().ResetRenderTarget();
		GetRenderPipeline().ResetViewport();

		// Pass #2 : Render scene with shadow map

		BindShadowMapChunk();
		GetRenderPipeline().BindCascadeBufferChunk(m_cascade_shadow_buffer_chunk_);
	}

	void ShadowManager::ClearShadowMaps()
	{
		for (const auto& buffer : m_graphic_shadow_buffer_ | std::views::values)
		{
			GetRenderPipeline().ResetShadowMap(buffer.depth_stencil_view.Get());
		}
	}

	void ShadowManager::Render(const float& dt)
	{
		// Pass 2 Ends, reset shadow map

		GetRenderPipeline().UnbindShadowMap(m_lights_.size());
		ClearShadowMaps();
		ClearShadowBufferChunk();
		GetRenderPipeline().ResetDepthStencilState();
	}

	void ShadowManager::FixedUpdate(const float& dt)
	{
	}

	void ShadowManager::BuildShadowMap(Scene& scene) const
	{
		for (int i = 0; i < LAYER_MAX; ++i)
		{
			if (i == LAYER_LIGHT || i == LAYER_UI || i == LAYER_CAMERA || i == LAYER_SKYBOX) continue;
		
			for (const auto& objects : scene.GetGameObjects((eLayerType)i))
			{
				if (const auto locked = objects.lock())
				{
					const auto tr = locked->GetComponent<Engine::Component::Transform>().lock();
					const auto mesh = locked->GetResource<Engine::Resources::Mesh>().lock();

					if (tr && mesh)
					{
						tr->Render(placeholder);
						mesh->Render(placeholder);
					}
				}
			}
		}
	}

	void ShadowManager::CreateSubfrusta(const Matrix& projection, float start, float end, Subfrusta& subfrusta) const
	{
		BoundingFrustum frustum (projection);

		frustum.Near = start;
		frustum.Far = end;

	    static constexpr XMVECTORU32 vGrabY = {0x00000000,0xFFFFFFFF,0x00000000,0x00000000};
	    static constexpr XMVECTORU32 vGrabX = {0xFFFFFFFF,0x00000000,0x00000000,0x00000000};

		const Vector4 rightTopV = {frustum.RightSlope, frustum.TopSlope, 1.f, 1.f};
		const Vector4 leftBottomV = {frustum.LeftSlope, frustum.BottomSlope, 1.f, 1.f};
		const Vector4 nearV = {frustum.Near, frustum.Near, frustum.Near, 1.f};
		const Vector4 farV = {frustum.Far, frustum.Far, frustum.Far, 1.f};

		const Vector4 rightTopNear = rightTopV * nearV;
		const Vector4 righTopFar = rightTopV * farV;
		const Vector4 LeftBottomNear = leftBottomV * nearV;
		const Vector4 LeftBottomFar = leftBottomV * farV;

		subfrusta.corners[0] = rightTopNear;
		subfrusta.corners[1] = XMVectorSelect(rightTopNear, LeftBottomNear, vGrabX);
		subfrusta.corners[2] = LeftBottomNear;
		subfrusta.corners[3] = XMVectorSelect(rightTopNear, LeftBottomNear, vGrabY);

		subfrusta.corners[4] = righTopFar;
		subfrusta.corners[5] = XMVectorSelect(righTopFar, LeftBottomFar, vGrabX);
		subfrusta.corners[6] = LeftBottomFar;
		subfrusta.corners[7] = XMVectorSelect(righTopFar, LeftBottomFar, vGrabY);
	}

	void ShadowManager::EvalCascadeVP(const Vector3& light_dir, CascadeShadowBuffer& buffer, const UINT light_index, CascadeShadowBufferChunk& chunk)
	{
		// https://cutecatgame.tistory.com/6

		if (const auto scene = GetSceneManager().GetActiveScene().lock())
		{
			if (const auto camera = scene->GetMainCamera().lock())
			{
				const float near_plane = g_screen_near;
				const float far_plane = g_screen_far;

				const float cascadeEnds[]
				{
					near_plane, 10.f, 80.f, far_plane
				};

				// for cascade shadow mapping, total 3 parts are used.
				// (near, 6), (6, 18), (18, far)
				for (auto i = 0; i < g_max_shadow_cascades; ++i)
				{
					// frustum = near points 4 + far points 4
					CreateSubfrusta(
						camera->GetProjectionMatrix(),
						cascadeEnds[i], 
						cascadeEnds[i + 1], 
						m_subfrusta_[i]);

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
						radius = std::max(radius, distance);
					}
					
					radius = std::ceil(radius * 16.f) / 16.f;

					Vector3 maxExtent = Vector3{radius, radius, radius};
					Vector3 minExtent = -maxExtent;
					const Vector3 cascadeExtents = maxExtent - minExtent;

					const auto pos = center + (light_dir * std::fabsf(minExtent.z));

					// DX11 uses column major matrix

					buffer.shadow.view[i] =
						XMMatrixTranspose(XMMatrixLookAtLH(
						pos, 
						Vector3(center), 
						Vector3::Up));

					buffer.shadow.proj[i] =
						XMMatrixTranspose(XMMatrixOrthographicOffCenterLH(
							minExtent.x, 
							maxExtent.x, 
							minExtent.y, 
							maxExtent.y, 
							0.f, 
							cascadeExtents.z));

					buffer.shadow.end_clip_spaces[i] = Vector4{0.f, 0.f, cascadeEnds[i + 1], 1.f};
					buffer.shadow.end_clip_spaces[i] = Vector4::Transform(buffer.shadow.end_clip_spaces[i], camera->GetProjectionMatrix()); // use z axis
				}
			}
		}
	}

	void ShadowManager::RegisterLight(const WeakLight& light)
	{
		m_lights_.insert(light);

		GetRenderPipeline().InitializeShadowBuffer(m_graphic_shadow_buffer_[light]);

		m_cascade_vp_buffer_[light] = {};
	}

	void ShadowManager::UnregisterLight(const WeakLight& light)
	{
		int idx = std::distance(m_lights_.begin(), m_lights_.find(light));

		m_lights_.erase(light);
		m_graphic_shadow_buffer_.erase(light);
		m_cascade_vp_buffer_.erase(light);

		GetRenderPipeline().SetLight(idx, Matrix::Identity, Color{0.0f, 0.0f, 0.0f, 0.0f});
	}
}
