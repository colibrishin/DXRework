#pragma once
#include "Source/Runtime/CoreSingleton/Public/Singleton.hpp"
#include "Source/Runtime/Core/Delegation/Public/Delegation.hpp"
#include "Source/Runtime/Core/TaskScheduler/Public/TaskScheduler.h"

DEFINE_DELEGATE(OnSceneActive, Engine::Weak<Engine::Scene>);
DEFINE_DELEGATE(OnSceneRemoved, Engine::Weak<Engine::Scene>);

POLYMORPHIC_MANAGER_TYPE_MAP(ENGINE_CORE_API, Engine::Managers::SceneManager)

namespace Engine::Managers
{
	class ENGINE_CORE_API SceneManager final : public Abstracts::Singleton<SceneManager>
	{
	public:
		DelegateOnSceneActive onSceneActive;
		DelegateOnSceneRemoved onSceneRemoved;

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
		void Update(const float dt) override;
		void PreUpdate(const float dt) override;
		void PreRender(const float dt) override;
		void PostUpdate(const float dt) override;
		void Render(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostRender(const float dt) override;
		void OnUIUpdate(UIContext* const parent, const float dt) override;

#if WITH_EDITOR
		using NewFunctionSignature = const std::function<void()>;
		using AddFunctionSignature = const std::function<void()>;
		using LoadFunctionSignature = std::function<void(const std::string_view)>;

		void RegisterNewMenuItem(std::string_view name, const NewFunctionSignature& predicate);
		void RegisterAddMenuItem(std::string_view name, const AddFunctionSignature& predicate);
		void RegisterLoadMenuItem(std::string_view name, const LoadFunctionSignature& predicate);
		void UnregisterNewMenuItem(std::string_view name);
		void UnregisterAddMenuItem(std::string_view name);
		void UnregisterLoadMenuItem(std::string_view name);

	private:
		// Assuming string address is constant.
		std::unordered_map<std::string_view, NewFunctionSignature> m_custom_new_function_;
		std::unordered_map<std::string_view, AddFunctionSignature> m_custom_add_function_;
		std::unordered_map<std::string_view, LoadFunctionSignature> m_custom_load_function_;
#endif

	private:
		friend struct SingletonDeleter;
		~SceneManager() override = default;

		// Internal usage of add scene, used for un-deducible type (runtime).
		void AddScene(const Weak<Scene>& ptr_scene);

		void SetActiveFinalize(const Weak<Scene>& it);

		void RemoveSceneFinalize(const Strong<Scene>& scene, const std::string& name);

		Weak<Scene>                m_active_scene_{};
		std::vector<Strong<Scene>> m_scenes_{};
	};
} // namespace Engine::Managers