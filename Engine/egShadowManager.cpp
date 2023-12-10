#include "pch.hpp"
#include "egShadowManager.hpp"

#include "egObject.hpp"
#include "egResourceManager.hpp"
#include "egSceneManager.hpp"
#include "egScene.hpp"

#include "egTransform.hpp"
#include "egMesh.hpp"
#include "egCamera.hpp"

namespace Engine::Manager::Graphics
{
	void ShadowManager::Initialize()
	{
		m_vs_stage1 = GetResourceManager().GetResource<Graphic::VertexShader>("vs_cascade_shadow_stage1").lock();
		m_gs_stage1 = GetResourceManager().GetResource<Graphic::GeometryShader>("gs_cascade_shadow_stage1").lock();
		m_ps_stage1 = GetResourceManager().GetResource<Graphic::PixelShader>("ps_cascade_shadow_stage1").lock();

		m_vs_stage2 = GetResourceManager().GetResource<Graphic::VertexShader>("vs_cascade_shadow_stage2").lock();
		m_ps_stage2 = GetResourceManager().GetResource<Graphic::PixelShader>("ps_cascade_shadow_stage2").lock();
	}

	void ShadowManager::PreUpdate(const float& dt)
	{
	}

	void ShadowManager::Update(const float& dt)
	{
	}

	void ShadowManager::PreRender(const float& dt)
	{
	}

	void ShadowManager::Render(const float& dt)
	{
		if (const auto scene = GetSceneManager().GetActiveScene().lock())
		{
			const auto& lights = scene->GetGameObjects((eLayerType)LAYER_LIGHT);

			for (const auto& light : lights)
			{
				if (const auto locked = light.lock())
				{
					GetRenderPipeline().TargetShadowMap();

					locked->Render(0.f);
					BuildShadowMap(*scene);

					GetRenderPipeline().ResetRenderTarget();
					GetRenderPipeline().BindShadowMap();

					ApplyShadow(*scene);

					GetRenderPipeline().UnbindShadowMap();
					GetRenderPipeline().ResetShadowMap();
					GetRenderPipeline().SetFillState();
				}
			}
		}
	}

	void ShadowManager::FixedUpdate(const float& dt)
	{
	}

	void ShadowManager::BuildShadowMap(Scene& scene) const
	{
		// no need to use delta time
		constexpr float placeholder = 0.f;

		// set up the shaders for stage 1
		m_vs_stage1->Render(placeholder);
		m_gs_stage1->Render(placeholder);
		m_ps_stage1->Render(placeholder);

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

		GetRenderPipeline().ResetShaders();
	}

	void ShadowManager::ApplyShadow(Scene& scene) const
	{
		// no need to use delta time
		constexpr float placeholder = 0.f;

		// set up the shaders for stage 2
		m_vs_stage2->Render(placeholder);
		m_ps_stage2->Render(placeholder);

		GetRenderPipeline().BindShadowSampler();

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
						GetRenderPipeline().SetWorldMatrix(tr->m_transform_buffer_, SHADER_PIXEL);
						tr->Render(placeholder);
						mesh->Render(placeholder);
					}
				}
			}
		}

		GetRenderPipeline().ResetSampler(SHADER_PIXEL);
		GetRenderPipeline().ResetShaders();
	}

	void ShadowManager::GetCascadeShadow(const Vector3& light_dir, Vector4 position[3], Matrix view[3], Matrix projection[3], Vector4 clip[3]) const
	{
		// https://cutecatgame.tistory.com/6

		if (const auto scene = GetSceneManager().GetActiveScene().lock())
		{
			if (const auto camera = scene->GetMainCamera().lock())
			{
				const auto view_inv = camera->GetViewMatrix().Invert();
				const float fov = g_fov;
				const float aspect = GetD3Device().GetAspectRatio();
				const float near_plane = g_screen_near;
				const float far_plane = g_screen_far;

				const float tan_hf_v = std::tanf(XMConvertToRadians(fov / 2.f));
				const float tan_hf_h = tan_hf_v * aspect;

				const float cascadeEnds[]
				{
					near_plane, 6.f, 18.f, far_plane
				};

				// for cascade shadow mapping, total 3 parts are used.
				// (near, 6), (6, 18), (18, far)
				for (auto i = 0; i < g_max_shadow_cascades; ++i)
				{
					const float xn = cascadeEnds[i] * tan_hf_h;
					const float xf = cascadeEnds[i + 1] * tan_hf_h;

					const float yn = cascadeEnds[i] * tan_hf_v;
					const float yf = cascadeEnds[i + 1] * tan_hf_v;

					// frustum = near points 4 + far points 4
					Vector4 current_corner[8] =
					{
						// near plane
						{xn, yn, cascadeEnds[i], 1.f},
						{-xn, yn, cascadeEnds[i], 1.f},
						{xn, -yn, cascadeEnds[i], 1.f},
						{-xn, -yn, cascadeEnds[i], 1.f},

						// far plane
						{xf, yf, cascadeEnds[i + 1], 1.f},
						{-xf, yf, cascadeEnds[i + 1], 1.f},
						{xf, -yf, cascadeEnds[i + 1], 1.f},
						{-xf, -yf, cascadeEnds[i + 1], 1.f}
					};

					Vector4 center{};
					for (auto& corner : current_corner)
					{
						corner = Vector4::Transform(corner, view_inv);
						center += corner;
					}

					// Get center by averaging
					center /= 8.f;

					float radius = 0.f;
					for (const auto& corner : current_corner)
					{
						float distance = Vector4::Distance(center, corner);
						radius = std::max(radius, distance);
					}
					
					radius = std::ceil(radius * 16.f) / 16.f;

					Vector3 maxExtent = Vector3{radius, radius, radius};
					Vector3 minExtent = -maxExtent;

					position[i] = center + (light_dir * minExtent.z);

					view[i] = XMMatrixLookAtLH(position[i], Vector3(center), Vector3::Up);

					const Vector3 cascadeExtents = maxExtent - minExtent;

					projection[i] = XMMatrixOrthographicOffCenterLH(minExtent.x, maxExtent.x, minExtent.y, maxExtent.y, 0.f, cascadeExtents.z);

					clip[i] = Vector4{0.f, 0.f, cascadeEnds[i + 1], 1.f};
					clip[i] = Vector4::Transform(clip[i], camera->GetProjectionMatrix()); // use z axis
				}
			}
		}
	}
}
