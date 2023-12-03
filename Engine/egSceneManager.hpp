#pragma once
#include "egManager.hpp"
#include "egScene.hpp"
#include <set>
#include <memory>

#include "egDebugger.hpp"
#include "egTaskScheduler.hpp"

namespace Engine::Manager
{
	class SceneManager final : public Abstract::Singleton<SceneManager>
	{
	public:
		explicit SceneManager(SINGLETON_LOCK_TOKEN) : Singleton() {}

		WeakScene GetActiveScene() const { return m_active_scene_; }

		template <typename T, typename... Args>
		void AddScene(Args&&... args)
		{
			auto scene = Instantiate<T>(std::forward<Args>(args)...);
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
					return boost::dynamic_pointer_cast<T>(scene) != nullptr;
				}
			);

			if (it != m_scenes_.end())
			{
				m_active_scene_ = *it;
			}
		}

		WeakScene GetScene(EntityID id) const
		{
			auto it = std::find_if(
				m_scenes_.begin(),
				m_scenes_.end(),
				[id](const auto& scene)
				{
					return scene->GetID() == id;
				}
			);

			if (it != m_scenes_.end())
				return *it;

			return {};
		}

		template <typename T>
		void RemoveScene()
		{
			auto it = std::find_if(
				m_scenes_.begin(),
				m_scenes_.end(),
				[](const auto& scene)
				{
					return boost::dynamic_pointer_cast<T>(scene) != nullptr;
				}
			);

			if (it != m_scenes_.end())
			{
				if (*it == m_active_scene_.lock())
				{
					Debugger::GetInstance().Log(L"Warning: Active scene has been removed.");
					m_active_scene_.reset();
				}
					
				m_scenes_.erase(it);
			}
		}

		void Initialize() override;
		void Update(const float& dt) override;
		void PreUpdate(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void FixedUpdate(const float& dt) override;

	private:
		WeakScene m_active_scene_;
		std::set<StrongScene> m_scenes_;

	};

	inline void SceneManager::Initialize()
	{
	}

	inline void SceneManager::Update(const float& dt)
	{
		m_active_scene_.lock()->Update(dt);
	}

	inline void SceneManager::PreUpdate(const float& dt)
	{
		m_active_scene_.lock()->PreUpdate(dt);
	}

	inline void SceneManager::PreRender(const float& dt)
	{
		m_active_scene_.lock()->PreRender(dt);
	}

	inline void SceneManager::Render(const float& dt)
	{
		m_active_scene_.lock()->Render(dt);
	}

	inline void SceneManager::FixedUpdate(const float& dt)
	{
		m_active_scene_.lock()->FixedUpdate(dt);
	}
}
