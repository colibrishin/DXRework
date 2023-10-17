#pragma once
#include "egManager.hpp"
#include "egScene.hpp"
#include <set>
#include <memory>

namespace Engine::Manager
{
	using WeakScene = std::weak_ptr<Scene>;
	using ConcreteScenePtr = std::shared_ptr<Scene>;

	class SceneManager final : Abstract::Manager
	{
	public:
		WeakScene GetActiveScene() const { return m_active_scene_; }

		template <typename T, typename... Args>
		void AddScene(Args&&... args)
		{
			auto scene = std::make_shared<T>(std::forward<Args>(args)...);
			scene->Initialize();
			m_scenes_.insert(scene);
		}

		template <typename T>
		void SetActive()
		{
			auto it = std::find_if(
						m_scenes_.begin(), 
							m_scenes_.end(), 
							[](const auto& scene)
				{
					return std::dynamic_pointer_cast<T>(scene) != nullptr;
				}
			);

			if (it != m_scenes_.end())
			{
				m_active_scene_ = *it;
			}
		}

		template <typename T>
		void RemoveScene()
		{
			auto it = std::find_if(
				m_scenes_.begin(), 
				m_scenes_.end(), 
				[](const auto& scene) 
				{
					return std::dynamic_pointer_cast<T>(scene) != nullptr;
				}
			);

			if (it != m_scenes_.end())
			{
				m_scenes_.erase(it);
			}
		}

		void Initialize() override;
		void Update() override;
		void PreUpdate() override;
		void PreRender() override;
		void Render() override;

		static SceneManager* GetInstance()
		{
			if (m_instance_ == nullptr)
			{
				m_instance_ = std::unique_ptr<SceneManager>(new SceneManager);
				m_instance_->Initialize();
			}

			return m_instance_.get();
		}

	private:
		SceneManager() = default;

		inline static std::unique_ptr<SceneManager> m_instance_ = nullptr;

		WeakScene m_active_scene_;
		std::set<ConcreteScenePtr> m_scenes_;
	};

	inline void SceneManager::Initialize()
	{
	}

	inline void SceneManager::Update()
	{
		m_active_scene_.lock()->Update();
	}

	inline void SceneManager::PreUpdate()
	{
		m_active_scene_.lock()->PreUpdate();
	}

	inline void SceneManager::PreRender()
	{
		m_active_scene_.lock()->PreRender();
	}

	inline void SceneManager::Render()
	{
		m_active_scene_.lock()->Render();
	}
}
