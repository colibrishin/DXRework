#include "../Public/SceneManager.hpp"

#include "Source/Runtime/Scene/Public/Scene.hpp"
#include "Source/Runtime/Managers/ShadowManager/Public/ShadowManager.hpp"
#include "Source/Runtime/Managers/Debugger/Public/Debugger.hpp"
#include "Source/Runtime/CoreObjects/Light/Public/Light.h"

namespace Engine::Managers
{
	void SceneManager::SetActiveFinalize(const Weak<Scene>& it)
	{
		Managers::ShadowManager::GetInstance().Reset();
		m_active_scene_ = it;

		if (const auto& scene = m_active_scene_.lock())
		{
			if (!it.lock()->IsInitialized())
			{
				scene->Initialize();
			}

			for (const auto& light :
			     scene->GetGameObjects(RESERVED_LAYER_LIGHT))
			{
				Managers::ShadowManager::GetInstance().RegisterLight
						(
						 light.lock()->GetSharedPtr<Objects::Light>()
						);
			}

			g_raytracing = scene->m_b_scene_raytracing_;
		}
	}

	void SceneManager::RemoveSceneFinalize(const Strong<Scene>& scene, const std::string& name)
	{
		if (scene == m_active_scene_.lock())
		{
			Managers::Debugger::GetInstance().Log("Warning: Active scene has been removed.");
			Managers::ShadowManager::GetInstance().Reset();
			m_active_scene_.reset();
		}

		std::erase_if
		(
			m_scenes_, [scene](const auto& v_scene)
			{
				return scene == v_scene;
			}
		);
	}

	void SceneManager::AddScene(const std::string& name)
	{
		const auto scene = boost::make_shared<Scene>();
		scene->SetName(name);
		m_scenes_.push_back(scene);
	}

	void SceneManager::SetActive(const std::string& name)
	{
		// Orders :
		// 1. At the start of the frame, scene will be set as active and initialized, resetting shadow manager, push back to the task queue if there is any object creation, which has the higher priority than the scene. it will be processed in the next frame.
		// 2. Scene is activated, passing through the first frame without any objects. This has the effect that averaging out the noticeable delta time spike.
		// 3. On the second frame, pushed objects are processed, and added to the scene.
		if (const auto scene = std::ranges::find_if
		(
			m_scenes_, [name](const auto& scene)
			{
				return scene->GetName() == name;
			}
		);
		scene != m_scenes_.end())
		{
			TaskScheduler::GetInstance().AddTask
			(
				TASK_ACTIVE_SCENE,
				{ *scene },
				[name, this](const std::vector<std::any>& params, float)
				{
					const auto s = std::any_cast<Strong<Scene>>(params[0]);
					SetActiveFinalize(s);
				}
			);
		}
	}

	inline Weak<Scene> SceneManager::GetScene(const std::string& name) const
	{
		const auto scene = std::ranges::find_if
		(
			m_scenes_, [name](const auto& scene)
			{
				return scene->GetName() == name;
			}
		);

		if (scene != m_scenes_.end())
		{
			return *scene;
		}

		return {};
	}

	void SceneManager::Initialize()
	{
		m_b_load_popup_ = false;
		AddScene("UntitledScene");
		SetActive("UntitledScene");
	}

	void SceneManager::Update(const float& dt)
	{
		if (const auto& scene = m_active_scene_.lock())
		{
			scene->Update(dt);
		}
	}

	void SceneManager::PreUpdate(const float& dt)
	{
		if (const auto& scene = m_active_scene_.lock())
		{
			scene->PreUpdate(dt);
		}
	}

	void SceneManager::PreRender(const float& dt)
	{
		if (const auto& scene = m_active_scene_.lock())
		{
			scene->PreRender(dt);
		}
	}

	void SceneManager::PostUpdate(const float& dt)
	{
		if (const auto& scene = m_active_scene_.lock())
		{
			scene->PostUpdate(dt);
		}
	}

	void SceneManager::Render(const float& dt)
	{
		if (const auto& scene = m_active_scene_.lock())
		{
			scene->Render(dt);
		}
	}

	void SceneManager::FixedUpdate(const float& dt)
	{
		if (const auto& scene = m_active_scene_.lock())
		{
			scene->FixedUpdate(dt);
		}
	}

	void SceneManager::PostRender(const float& dt)
	{
		if (const auto& scene = m_active_scene_.lock())
		{
			scene->PostRender(dt);
		}
	}

	void SceneManager::AddScene(const Weak<Scene>& ptr_scene)
	{
		if (const auto param_scene = ptr_scene.lock())
		{
			if (const auto target = std::ranges::find_if
						(
						 m_scenes_, [param_scene](const auto& v_s)
						 {
							 return v_s->GetName() == param_scene->GetName();
						 }
						);
				m_scenes_.end() != target)
			{
				Managers::TaskScheduler::GetInstance().AddTask
						(
						 TASK_SYNC_SCENE,
						 {*target, param_scene},
						 [this](const std::vector<std::any>& params, float)
						 {
							 const auto target = std::any_cast<Strong<Scene>>(params[0]);
							 const auto scene  = std::any_cast<Strong<Scene>>(params[1]);
							 target->synchronize(scene);
						 }
						);
			}
			else
			{
				m_scenes_.push_back(param_scene);
			}
		}
	}
} // namespace Engine::Manager
