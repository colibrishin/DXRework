#include "../Public/SceneManager.h"

#include "UIInterface.h"

#include "Source/Runtime/Core/Scene/Public/Scene.h"
#include "Source/Runtime/Core/Objects/Light/Public/Light.h"
#include "SceneManager.generated.h"
#include "Serialization.hpp"

#if WITH_DEBUG
#include "Source/Runtime/Core/Debugger/Public/Debugger.h"
#endif

namespace Engine::Managers
{
	void SceneManager::SetActiveFinalize(const Weak<Scene>& it)
	{
		m_active_scene_ = it;

		if (const auto& scene = m_active_scene_.lock())
		{
			if (!it.lock()->IsInitialized())
			{
				scene->Initialize();
			}

			//g_raytracing = scene->m_b_scene_raytracing_;
		}

		onSceneActive.Broadcast(m_active_scene_.lock());
	}

	void SceneManager::RemoveSceneFinalize(const Strong<Scene>& scene, const std::string& name)
	{
		if (scene == m_active_scene_.lock())
		{
#if WITH_DEBUG
			Managers::Debugger::GetInstance().Log("Warning: Active scene has been removed.", {1.f, 0.f, 0.f, 1.f});
#endif
			onSceneRemoved.Broadcast(m_active_scene_);
			m_active_scene_.reset();
		}

		const decltype(m_scenes_)::iterator& found_scene = std::ranges::find_if
		(
		 m_scenes_, [scene](const auto& v_scene)
			{
				return scene == v_scene;
			}
		);

		onSceneRemoved.Broadcast(*found_scene);
		m_scenes_.erase(found_scene);
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
		RegisterLoadMenuItem(Scene::StaticTypeName(), [](bool& managing_flag)
			{
				const auto& load_callback = [](const std::string_view name, const std::string_view path)
					{
						if (Strong<Scene> scene;
							Serializer::Deserialize<Scene>(path.data(), scene))
						{
							GetInstance().AddScene(scene);
							GetInstance().SetActive(scene->GetName());
						}
					};

				return UIHelpers::OpenLoadDialog<Scene, SceneManager>(managing_flag, {}, load_callback, {});
			});

		RegisterNewMenuItem(Scene::StaticTypeName(), [](bool& managing_flag) 
			{
				GetInstance().AddScene("UntitledScene");
				GetInstance().SetActive("UntitledScene");
				managing_flag = false;
			});

		AddScene("UntitledScene");
		SetActive("UntitledScene");
	}

	void SceneManager::Update(const float dt)
	{
		if (const auto& scene = m_active_scene_.lock())
		{
			scene->Update(dt);
		}
	}

	void SceneManager::PreUpdate(const float dt)
	{
		if (const auto& scene = m_active_scene_.lock())
		{
			scene->PreUpdate(dt);
		}
	}

	void SceneManager::PreRender(const float dt)
	{
		if (const auto& scene = m_active_scene_.lock())
		{
			scene->PreRender(dt);
		}
	}

	void SceneManager::PostUpdate(const float dt)
	{
		if (const auto& scene = m_active_scene_.lock())
		{
			scene->PostUpdate(dt);
		}
	}

	void SceneManager::Render(const float dt)
	{
		if (const auto& scene = m_active_scene_.lock())
		{
			scene->Render(dt);
		}
	}

	void SceneManager::FixedUpdate(const float dt)
	{
		if (const auto& scene = m_active_scene_.lock())
		{
			scene->FixedUpdate(dt);
		}
	}

	void SceneManager::PostRender(const float dt)
	{
		if (const auto& scene = m_active_scene_.lock())
		{
			scene->PostRender(dt);
		}
	}

	void SceneManager::OnUIUpdate(UIContext* const parent, const float dt)
	{
#if WITH_EDITOR
		UIInterface& ui = UIInterfaceAccessor::GetInterface();

		if (UIContext context = UIInterface::NewContext(ui.NewMainMenuBar({})))
		{
			context += ui.NewMenu({"New"});

			for (auto& [name, func] : m_custom_new_function_)
			{
				(context |= ui.NewMenuItem({name})).SetFunction([&]()
				{
					func.first = true;
				});
			}

			const auto& addTemplate = [&] <typename T, LayerSizeType Layer> ()
			{
				GetActiveScene().lock()->CreateGameObject<T>(Layer);
			};

			(context |= ui.NewMenuItem({ "Camera" })).SetFunction([&]()
				{
					addTemplate.operator() < Objects::Camera, RESERVED_LAYER_CAMERA > ();
				});

			(context |= ui.NewMenuItem({ "Light" })).SetFunction([&]()
				{
					addTemplate.operator() < Objects::Light, RESERVED_LAYER_LIGHT > ();
				});

			(context |= ui.NewMenuItem({ "Object" })).SetFunction([&]()
				{
					addTemplate.operator() < Object, RESERVED_LAYER_DEFAULT > ();
				});

			--context;

			context += ui.NewMenu({"Load"});

			for (auto& [name, func] : m_custom_load_function_)
			{
				(context |= ui.NewMenuItem({name})).SetFunction([&]()
				{
					func.first = true;
				});
			}

			--context;

			if (const auto& scene = m_active_scene_.lock())
			{
				scene->OnUIUpdate(&context, dt);
			}

			for (auto& [flag, func] : m_custom_new_function_ | std::views::values)
			{
				if (flag) 
				{
					func(flag);
				}
			}

			for (auto& [flag, func] : m_custom_load_function_ | std::views::values)
			{
				if (flag)
				{
					func(flag);
				}
			}
		}
#endif
	}

	bool SceneManager::IsPlaying() const
	{
		return m_b_playing_;
	}

#if WITH_EDITOR
	void SceneManager::RegisterNewMenuItem(std::string_view name, const UIHelpers::ManagedBooleanSignature& predicate)
	{
		if (!m_custom_new_function_.contains(name))
		{
			m_custom_new_function_[name] = { false, predicate };
		}
	}

	void SceneManager::RegisterLoadMenuItem(std::string_view name, const UIHelpers::ManagedBooleanSignature& predicate)
	{
		if (!m_custom_load_function_.contains(name))
		{
			m_custom_load_function_[name] = { false, predicate };
		}
	}

	void SceneManager::UnregisterLoadMenuItem(const std::string_view name)
	{
		if (m_custom_load_function_.contains(name))
		{
			m_custom_load_function_.erase(name);
		}
	}

	void SceneManager::UnregisterNewMenuItem(const std::string_view name)
	{
		if (m_custom_new_function_.contains(name))
		{
			m_custom_new_function_.erase(name);
		}
	}
#endif

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
