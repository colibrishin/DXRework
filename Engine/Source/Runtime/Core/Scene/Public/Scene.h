#pragma once
#include <any>
#include <ranges>
#include <boost/serialization/export.hpp>

#include "Source/Runtime/Core/Component/Public/Component.h"
#include "Source/Runtime/CoreEntity/Public/Renderable.h"
#include "Source/Runtime/Core/BoundingGetter/Public/BoundingGetter.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/TaskScheduler/Public/TaskScheduler.h"
#include "Source/Runtime/Core/Octree/Public/Octree.hpp"
#include "Source/Runtime/Core/Script/Public/Script.h"
#include "Source/Runtime/Core/Delegation/Public/Delegation.hpp"
#include "SingletonSpinLock/Public/SingletonSpinLock.h"


#include "Scene.generated.h"

#ifdef PHYSX_ENABLED
namespace physx
{
	class PxScene;
}
#endif

DEFINE_DELEGATE(OnObjectAdded, Engine::Weak<Engine::Abstracts::ObjectBase>);
DEFINE_DELEGATE(OnObjectRemoved, Engine::Weak<Engine::Abstracts::ObjectBase>);

namespace Engine
{
	EENUM()
	enum ENGINE_CORE_API eReservedLayerType
	{
		RESERVED_LAYER_DEFAULT,
		RESERVED_LAYER_LIGHT,
		RESERVED_LAYER_CAMERA,
		RESERVED_LAYER_ENVIRONMENT,
		RESERVED_LAYER_SKYBOX,
		RESERVED_LAYER_OBSERVER,
		RESERVED_LAYER_UI,
		RESERVED_LAYER_MAX
	};

	constexpr const char* g_reserved_layer_name[] = 
	{
		"Default",
		"Light",
		"Camera",
		"Environment",
		"Skybox",
		"Observer",
		"UI"
	};
	
	ECLASS(serialize)
	class ENGINE_CORE_API Scene : public Abstracts::Renderable
	{
		GENERATE_BODY
	public:
		DelegateOnObjectAdded onObjectAdded;
		DelegateOnObjectRemoved onObjectRemoved;

		Scene();
		Scene(const Scene& other) = default;
		~Scene() override;

		void DisableControllers();
		void AddObserver();
		void Initialize() final;

		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostRender(const float dt) override;
		void PostUpdate(const float dt) override;
		void OnUIUpdate(UIContext* const parent, const float dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		void                        SetMainActor(LocalActorID id);
		Weak<Abstracts::ObjectBase> GetMainActor() const;

		// Add Object to the scene.
		// If the object is bound to another scene or layer, it will be moved to this scene and layer.
		// Note that the object will be added finally at the next frame.
		template <typename T, typename ObjLock = std::enable_if_t<std::is_base_of_v<Abstracts::ObjectBase, T>>>
		void AddGameObject(LayerSizeType layer, const Strong<T>& obj)
		{
			const auto& downcast = obj->template GetSharedPtr<Abstracts::ObjectBase>();
			addGameObjectImpl(layer, downcast);
		}

		// Create Object and add it to the scene.
		// Note that the object will be added finally at the next frame.
		template <typename T, typename... Args, typename ObjLock = std::enable_if_t<std::is_base_of_v<
			          Abstracts::ObjectBase, T>>>
		Weak<T> CreateGameObject(LayerSizeType layer, Args&&... args)
		{
			// Create object, dynamic allocation from scene due to the access limitation.
			const auto& obj_t = boost::make_shared<T>(args...);
			const auto& obj   = obj_t->template GetSharedPtr<Abstracts::ObjectBase>();

			// Set internal information as this scene and layer, segmenting this process for
			// code re-usability.
			addGameObjectImpl(layer, obj);

			// yield the currently created object
			return obj_t;
		}

		void ChangeLayer(LayerSizeType to, GlobalEntityID id);

		void RemoveGameObject(GlobalEntityID id, LayerSizeType layer);

		Weak<Abstracts::ObjectBase> FindGameObject(GlobalEntityID id);
		Weak<Abstracts::ObjectBase> FindGameObjectByLocalID(LocalActorID id);

		ConcurrentWeakObjVec  GetGameObjectsConcurrent(LayerSizeType layer) const;
		WeakObjVec  GetGameObjects(LayerSizeType layer) const;
		Weak<Objects::Camera> GetMainCamera() const;

		const Octree<Weak<Abstracts::ObjectBase>, bounding_getter>& GetObjectTree();
		const Octree<Weak<Abstracts::ObjectBase>, bounding_getter>& GetCollisionTree();

		// Add cache component from the object.
		template <typename T, typename CompLock = std::enable_if_t<std::is_base_of_v<Abstracts::Component, T>>>
		void AddCacheComponent(const Strong<T>& component)
		{
			// If the component cannot be deduced, go with runtime.
			if constexpr (std::is_same_v<Abstracts::Component, T>)
			{
				Managers::TaskScheduler::GetInstance().AddTask
						(
						 TASK_CACHE_COMPONENT,
						 {GetSharedPtr<Scene>(), component},
						 [](const std::vector<std::any>& params, const float)
						 {
							 const auto& scene     = std::any_cast<Strong<Scene>>(params[0]);
							 const auto& component = std::any_cast<Strong<Abstracts::Component>>(params[1]);

							 scene->addCacheComponentImpl(component, component->GetTypeHash());
						 }
						);
			}
			else
			{
				Managers::TaskScheduler::GetInstance().AddTask
						(
						 TASK_CACHE_COMPONENT,
						 {GetSharedPtr<Scene>(), component},
						 [](const std::vector<std::any>& params, const float)
						 {
							 const auto& scene     = std::any_cast<Strong<Scene>>(params[0]);
							 const auto& component = std::any_cast<Strong<T>>(params[1]);

							 scene->addCacheComponentImpl(component, T::StaticTypeHash());
						 }
						);
			}
		}

		// Remove cache component from the object.
		template <typename T, typename CompLock = std::enable_if_t<std::is_base_of_v<Abstracts::Component, T>>>
		void RemoveCacheComponent(const Strong<T>& script)
		{
			// If the component cannot be deduced, go with runtime.
			if constexpr (std::is_same_v<Abstracts::Component, T>)
			{
				Managers::TaskScheduler::GetInstance().AddTask
						(
						 TASK_UNCACHE_COMPONENT,
						 {GetSharedPtr<Scene>(), script},
						 [](const std::vector<std::any>& params, const float)
						 {
							 const auto& scene     = std::any_cast<Strong<Scene>>(params[0]);
							 const auto& component = std::any_cast<Strong<Abstracts::Component>>(params[1]);

							 scene->removeCacheComponentImpl(component, component->GetTypeHash());
						 }
						);
			}
			else
			{
				Managers::TaskScheduler::GetInstance().AddTask
						(
						 TASK_UNCACHE_COMPONENT,
						 {GetSharedPtr<Scene>(), script},
						 [](const std::vector<std::any>& params, const float)
						 {
							 const auto& scene     = std::any_cast<Strong<Scene>>(params[0]);
							 const auto& component = std::any_cast<Strong<T>>(params[1]);

							 scene->removeCacheComponentImpl(component, T::StaticTypeHash());
						 }
						);
			}
		}

		// Add cache script from the object.
		template <typename T, typename ScriptLock = std::enable_if_t<std::is_base_of_v<Script, T>>>
		void AddCacheScript(const Strong<T>& script)
		{
			// If the component cannot be deduced, go with runtime.
			if constexpr (std::is_same_v<Script, T>)
			{
				Managers::TaskScheduler::GetInstance().AddTask
						(
						 TASK_CACHE_SCRIPT,
						 {GetSharedPtr<Scene>(), script},
						 [](const std::vector<std::any>& params, const float)
						 {
							 const auto& scene = std::any_cast<Strong<Scene>>(params[0]);
							 const auto& scp   = std::any_cast<Strong<Script>>(params[1]);

							 scene->addCacheScriptImpl(scp, scp->GetTypeHash());
						 }
						);
			}
			else
			{
				Managers::TaskScheduler::GetInstance().AddTask
						(
						 TASK_CACHE_COMPONENT,
						 {GetSharedPtr<Scene>(), script},
						 [](const std::vector<std::any>& params, const float)
						 {
							 const auto& scene     = std::any_cast<Strong<Scene>>(params[0]);
							 const auto& component = std::any_cast<Strong<T>>(params[1]);

							 scene->addCacheScriptImpl(component, T::StaticTypeHash());
						 }
						);
			}
		}

		// Remove cache script from the object.
		template <typename T, typename ScriptLock = std::enable_if_t<std::is_base_of_v<Script, T>>>
		void RemoveCacheScript(const Strong<T>& script)
		{
			// If the component cannot be deduced, go with runtime.
			if constexpr (std::is_same_v<Script, T>)
			{
				Managers::TaskScheduler::GetInstance().AddTask
						(
						 TASK_UNCACHE_SCRIPT,
						 {GetSharedPtr<Scene>(), script},
						 [](const std::vector<std::any>& params, const float)
						 {
							 const auto& scene = std::any_cast<Strong<Scene>>(params[0]);
							 const auto& scp   = std::any_cast<Strong<Script>>(params[1]);

							 scene->removeCacheScriptImpl(scp, scp->GetTypeHash());
						 }
						);
			}
			else
			{
				Managers::TaskScheduler::GetInstance().AddTask
						(
						 TASK_UNCACHE_SCRIPT,
						 {GetSharedPtr<Scene>(), script},
						 [](const std::vector<std::any>& params, const float)
						 {
							 const auto& scene = std::any_cast<Strong<Scene>>(params[0]);
							 const auto& scp   = std::any_cast<Strong<T>>(params[1]);

							 scene->removeCacheScriptImpl(scp, T::StaticTypeHash());
						 }
						);
			}
		}

		template <typename T>
		[[nodiscard]] ConcurrentWeakComVec GetCachedComponentsConcurrent() const
		{
			ConcurrentWeakComRootMap::const_accessor acc;

			if (m_concurrent_cached_components_.find(acc, T::StaticTypeHash()))
			{
				ConcurrentWeakComVec result;

				for (const auto& comp : acc->second | std::views::values)
				{
					result.push_back(comp);
				}

				return result;
			}

			return {};
		}

		template <typename T>
		[[nodiscard]] WeakComVec GetCachedComponents() const
		{
			SpinLockToken token = SingletonSpinLock::GetInstance().Lock(m_component_lock_);
			if (m_cached_components_.contains(T::StaticTypeHash()))
			{
				auto& found = m_cached_components_.at(T::StaticTypeHash());
				WeakComVec result;
				for (const auto& comp : m_cached_components_.at(T::StaticTypeHash()) | std::views::values)
				{
					result.emplace_back(comp);
				}
				return result;
			}

			return {};
		}

		template <typename T>
		[[nodiscard]] ConcurrentWeakScpVec GetCachedScriptsConcurrent() const
		{
			ConcurrentWeakScpRootMap::const_accessor acc;

			if (m_cached_scripts_.find(acc, T::StaticTypeHash()))
			{
				ConcurrentWeakScpVec result;

				for (const auto& scp : acc->second | std::views::values)
				{
					result.push_back(scp);
				}

				return result;
			}

			return {};
		}

		template <typename T>
		[[nodiscard]] WeakScpVec GetCachedScripts() const
		{
			SpinLockToken token = SingletonSpinLock::GetInstance().Lock(m_script_lock_);
			if (m_cached_scripts_.contains(T::StaticTypeHash()))
			{
				auto& found = m_cached_scripts_.at(T::StaticTypeHash());
				WeakScpVec result;
				for (const auto& comp : m_cached_scripts_.at(T::StaticTypeHash()) | std::views::values)
				{
					result.emplace_back(comp);
				}
				return result;
			}

			return {};
		}

		const bool (& GetCollisionMask() const)[RESERVED_LAYER_MAX + CFG_LAYER_COUNT][RESERVED_LAYER_MAX + CFG_LAYER_COUNT];
		void          UpdateCollisionMask(const bool collision_mask[RESERVED_LAYER_MAX + CFG_LAYER_COUNT][RESERVED_LAYER_MAX + CFG_LAYER_COUNT]);

		Strong<Layer> operator[](const size_t idx) const
		{
			return m_layers_[idx];
		}

		Strong<Layer> at(const size_t idx) const
		{
			return m_layers_[idx];
		}

		auto begin() noexcept
		{
			return m_layers_.begin();
		}

		auto end() noexcept
		{
			return m_layers_.end();
		}

		auto begin() const noexcept
		{
			return m_layers_.begin();
		}

		auto end() const noexcept
		{
			return m_layers_.end();
		}

		auto cbegin() const noexcept
		{
			return m_layers_.cbegin();
		}

		auto cend() const noexcept
		{
			return m_layers_.cend();
		}

		auto size() const noexcept
		{
			return m_layers_.size();
		}

	private:
		friend class Managers::SceneManager;

		void AssignLocalIDToObject(const Strong<Abstracts::ObjectBase>& obj);

		// Set the scene and layer to the object, and schedule the object to be added at the next frame.
		void addGameObjectImpl(LayerSizeType layer, const Strong<Abstracts::ObjectBase>& obj);
		// Add cache component from the object.
		void addCacheComponentImpl(const Strong<Abstracts::Component>& component, ComponentType type);
		// Remove cache component from the object.
		void removeCacheComponentImpl(const Strong<Abstracts::Component>& component, ComponentType type);

		// Add cache script from the object.
		void addCacheScriptImpl(const Strong<Script>& script, const ScriptType type);
		// Remove cache script from the object.
		void removeCacheScriptImpl(const Strong<Script>& script, const ScriptType type);

		// Functions for the next frame.

		// Add the object from the scene finally. this function should be called at the next frame.
		void AddObjectFinalize(LayerSizeType layer, const Strong<Abstracts::ObjectBase>& obj);
		// Remove the object from the scene finally. this function should be called at the next frame.
		void RemoveObjectFinalize(GlobalEntityID id, LayerSizeType layer);
		void initializeFinalize();

		void synchronize(const Weak<Scene>& ptr_scene);

		EPROPERTY()
		bool m_b_scene_raytracing_;

		EPROPERTY()
		LocalActorID m_main_camera_local_id_;

		EPROPERTY()
		LocalActorID m_main_actor_local_id_;

		EPROPERTY()
		std::vector<Strong<Layer>> m_layers_;

		EPROPERTY()
		bool m_collision_mask_[RESERVED_LAYER_MAX + CFG_LAYER_COUNT][RESERVED_LAYER_MAX + CFG_LAYER_COUNT]{};

#if WITH_EDITOR
		bool m_b_dialog_opened_ = true;
		std::string m_layer_list_box_name_;
#endif

		Weak<Abstracts::ObjectBase> m_observer_;
		Weak<Objects::Camera>       m_mainCamera_;
		Weak<Abstracts::ObjectBase> m_main_actor_;

		LocalGlobalIDMap            m_assigned_actor_ids_;

		WeakObjGlobalMap                                     m_cached_objects_;
		WeakComRootMap                                       m_cached_components_;
		WeakScpRootMap                                       m_cached_scripts_;

		SpinLockTicket m_object_lock_;
		SpinLockTicket m_component_lock_;
		SpinLockTicket m_script_lock_;

		ConcurrentWeakObjGlobalMap                           m_concurrent_cached_objects_;
		ConcurrentWeakComRootMap                             m_concurrent_cached_components_;
		ConcurrentWeakScpRootMap                             m_concurrent_cached_scripts_;
		
		Octree<Weak<Abstracts::ObjectBase>, bounding_getter> m_object_position_tree_;
		Octree<Weak<Abstracts::ObjectBase>, bounding_getter> m_object_collision_tree_;

		static std::atomic<bool> s_debug_observer_;

#ifdef PHYSX_ENABLED
	public:
		physx::PxScene* GetPhysXScene() const;
		void CleanupPhysX();

	private:
		void InitializePhysX();
		physx::PxScene*	m_physics_scene_;
#endif
	};
} // namespace Engine
