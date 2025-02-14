#pragma once
#include "Source/Runtime/CoreSingleton/Public/Singleton.h"
#include "Source/Runtime/Core/Delegation/Public/Delegation.hpp"
#include "Source/Runtime/Core/TaskScheduler/Public/TaskScheduler.h"

#include "UIHelpers.h"
#include "SceneManager.generated.h"

DEFINE_DELEGATE(OnSceneActive, Engine::Weak<Engine::Scene>);
DEFINE_DELEGATE(OnSceneRemoved, Engine::Weak<Engine::Scene>);

namespace Engine::Managers
{
	ECLASS()
	class ENGINE_CORE_API SceneManager final : public Abstracts::Singleton<SceneManager>
	{
		GENERATE_BODY
	public:
		DelegateOnSceneActive onSceneActive;
		DelegateOnSceneRemoved onSceneRemoved;

		explicit SceneManager(SINGLETON_LOCK_TOKEN) {}

		[[nodiscard]] Weak<Scene> GetActiveScene() const
		{
			return m_active_scene_;
		}

		void AddScene(const std::string& name);
		void SetActive(const std::string& name);
		[[nodiscard]] Weak<Scene> GetScene(const std::string& name) const;
		[[nodiscard]] const std::vector<Strong<Scene>>& GetScenes() const;

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
		void Update(const float dt) override;
		void PreUpdate(const float dt) override;
		void PreRender(const float dt) override;
		void PostUpdate(const float dt) override;
		void Render(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostRender(const float dt) override;
		void OnUIUpdate(UIContext* const parent, const float dt) override;
		bool IsPlaying() const;

#if WITH_EDITOR
		void RegisterNewMenuItem(std::string_view name, const UIHelpers::ManagedBooleanSignature& predicate);
		void RegisterLoadMenuItem(std::string_view name, const UIHelpers::ManagedBooleanSignature& predicate);
		void UnregisterNewMenuItem(std::string_view name);
		void UnregisterLoadMenuItem(std::string_view name);

	private:
		// Assuming string address is constant.
		UIHelpers::ManagedBoolAndFuncMap<std::string_view> m_custom_new_function_;
		UIHelpers::ManagedBoolAndFuncMap<std::string_view> m_custom_load_function_;
#endif

	private:
		SceneManager() = default;
		friend struct SingletonDeleter;
		~SceneManager() override = default;

		// Internal usage of add scene, used for un-deducible type (runtime).
		void AddScene(const Weak<Scene>& ptr_scene);

		void SetActiveFinalize(const Weak<Scene>& it);

		void RemoveSceneFinalize(const Strong<Scene>& scene, const std::string& name);

		Weak<Scene>                m_active_scene_{};
		std::vector<Strong<Scene>> m_scenes_{};
		
		bool m_b_playing_ = false;
	};
} // namespace Engine::Managers