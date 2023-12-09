#include "pch.hpp"
#include "egShadowManager.hpp"

#include "egObject.hpp"
#include "egResourceManager.hpp"
#include "egSceneManager.hpp"
#include "egScene.hpp"

#include "egTransform.hpp"
#include "egMesh.hpp"

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
					GetRenderPipeline().SetFillState();

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
}
