#include "pch.hpp"
#include "egShadowManager.hpp"

#include "egObject.hpp"
#include "egResourceManager.hpp"
#include "egSceneManager.hpp"
#include "egScene.hpp"

#include "egTransform.hpp"
#include "egMesh.hpp"
#include "egCamera.hpp"
#include "egProjectionFrustum.hpp"

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
			if (i == LAYER_LIGHT || i == LAYER_UI || i == LAYER_CAMERA) continue;

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
			if (i == LAYER_LIGHT || i == LAYER_UI || i == LAYER_CAMERA) continue;

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

	void ShadowManager::CreateFrusta(const Matrix& projection, float start, float end, Vector4 cornerPoints[8]) const
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

		cornerPoints[0] = rightTopNear;
		cornerPoints[1] = XMVectorSelect(rightTopNear, LeftBottomNear, vGrabX);
		cornerPoints[2] = LeftBottomNear;
		cornerPoints[3] = XMVectorSelect(rightTopNear, LeftBottomNear, vGrabY);

		cornerPoints[4] = righTopFar;
		cornerPoints[5] = XMVectorSelect(righTopFar, LeftBottomFar, vGrabX);
		cornerPoints[6] = LeftBottomFar;
		cornerPoints[7] = XMVectorSelect(righTopFar, LeftBottomFar, vGrabY);
	}

	void ShadowManager::GetCascadeShadow(const Vector3& light_dir, Vector4 position[3], Matrix view[3], Matrix projection[3], Vector4 clip[3]) const
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
					near_plane, 6.f, 18.f, far_plane
				};

				// for cascade shadow mapping, total 3 parts are used.
				// (near, 6), (6, 18), (18, far)
				for (auto i = 0; i < g_max_shadow_cascades; ++i)
				{
					// frustum = near points 4 + far points 4
					Vector4 current_corner[8]{};
					CreateFrusta(
						camera->GetProjectionMatrix(),
						cascadeEnds[i], 
						cascadeEnds[i + 1], 
						current_corner);

					const auto view_inv = camera->GetViewMatrix().Invert();

					// transform to world space
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
