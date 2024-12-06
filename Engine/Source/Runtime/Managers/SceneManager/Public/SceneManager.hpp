#pragma once
#include "Source/Runtime/Core/Singleton/Public/Singleton.hpp"
#include "Source/Runtime/Managers/TaskScheduler/Public/TaskScheduler.h"

namespace Engine::Managers
{
	class SceneManager final : public Abstracts::Singleton<SceneManager>
	{
	public:
		explicit SceneManager(SINGLETON_LOCK_TOKEN) {}

		Weak<Scene> GetActiveScene() const
		{
			return m_active_scene_;
		}

		void AddScene(const std::string& name);
		void SetActive(const std::string& name);
		Weak<Scene> GetScene(const std::string& name) const;

		template <typename T>
		void RemoveScene(const std::string& name)
		{
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
						 TASK_REM_SCENE,
						 {*scene, name},
						 [this](const std::vector<std::any>& params, float)
						 {
							 const auto scene = std::any_cast<Strong<Scene>>(params[0]);
							 const auto name  = std::any_cast<std::string>(params[1]);
							 RemoveSceneFinalize(scene, name);
						 }
						);
			}
		}

		void Initialize() override;
		void Update(const float& dt) override;
		void PreUpdate(const float& dt) override;
		void PreRender(const float& dt) override;
		void PostUpdate(const float& dt) override;
		void Render(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostRender(const float& dt) override;

	private:
		friend struct SingletonDeleter;
		~SceneManager() override = default;

		// Internal usage of add scene, used for un-deducible type (runtime).
		void AddScene(const Weak<Scene>& ptr_scene);

		void SetActiveFinalize(const Weak<Scene>& it);
		void OpenLoadPopup();

		void RemoveSceneFinalize(const Strong<Scene>& scene, const std::string& name);

		bool m_b_load_popup_ = false;

		Weak<Scene>                m_active_scene_;
		std::vector<Strong<Scene>> m_scenes_;
	};
} // namespace Engine::Managers