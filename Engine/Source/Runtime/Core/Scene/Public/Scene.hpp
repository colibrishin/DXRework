#pragma once
#include <any>
#include <ranges>
#include <boost/serialization/export.hpp>

#include "Source/Runtime/Core/Component/Public/Component.h"
#include "Source/Runtime/Core/Renderable/Public/Renderable.h"
#include "Source/Runtime/Core/BoundingGetter/Public/BoundingGetter.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/TaskScheduler/Public/TaskScheduler.h"
#include "Source/Runtime/Core/Octree/Public/Octree.hpp"
#include "Source/Runtime/Core/Script/Public/Script.h"
#include "Source/Runtime/Core/Delegation/Public/Delegation.hpp"

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
	enum CORE_API eReservedLayerType
	{
		RESERVED_LAYER_DEFAULT,
		RESERVED_LAYER_LIGHT,
		RESERVED_LAYER_CAMERA,
		RESERVED_LAYER_ENVIRONMENT,
		RESERVED_LAYER_SKYBOX,
		RESERVED_LAYER_OBSERVER,
		RESERVED_LAYER_UI,
	};

	class CORE_API Scene : public Abstracts::Renderable
	{
	public:
		DelegateOnObjectAdded onObjectAdded;
		DelegateOnObjectRemoved onObjectRemoved;

		Scene();
		Scene(const Scene& other) = default;
		~Scene() override;

		void DisableControllers();
		void AddObserver();
		void Initialize() final;

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostRender(const float& dt) override;
		void PostUpdate(const float& dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		void                        SetMainActor(LocalActorID id);
		Weak<Abstracts::ObjectBase> GetMainActor() const;

		// Add Object to the scene.
		// If the object is bound to another scene or layer, it will be moved to this scene and layer.
		// Note that the object will be added finally at the next frame.
		template <typename T, typename ObjLock = std::enable_if_t<std::is_base_of_v<Abstracts::ObjectBase, T>>>
		void AddGameObject(LayerSizeType layer, const boost::shared_ptr<T>& obj)
		{
			const auto& downcast = obj->template GetSharedPtr<Abstracts::ObjectBase>();
			addGameObjectImpl(layer, downcast);
		}

		// Create Object and add it to the scene.
		// Note that the object will be added finally at the next frame.
		template <typename T, typename... Args, typename ObjLock = std::enable_if_t<std::is_base_of_v<
			          Abstracts::ObjectBase, T>>>
		boost::weak_ptr<T> CreateGameObject(LayerSizeType layer, Args&&... args)
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

		Weak<Abstracts::ObjectBase> FindGameObject(GlobalEntityID id) const;
		Weak<Abstracts::ObjectBase> FindGameObjectByLocalID(LocalActorID id) const;

		ConcurrentWeakObjVec  GetGameObjects(LayerSizeType layer) const;
		Weak<Objects::Camera> GetMainCamera() const;

		const Octree<Weak<Abstracts::ObjectBase>, bounding_getter>& GetObjectTree();

		// Add cache component from the object.
		template <typename T, typename CompLock = std::enable_if_t<std::is_base_of_v<Abstracts::Component, T>>>
		void AddCacheComponent(const boost::shared_ptr<T>& component)
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

							 scene->addCacheComponentImpl(component, component->GetComponentType());
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
							 const auto& component = std::any_cast<boost::shared_ptr<T>>(params[1]);

							 scene->addCacheComponentImpl(component, which_component<T>::value);
						 }
						);
			}
		}

		// Remove cache component from the object.
		template <typename T, typename CompLock = std::enable_if_t<std::is_base_of_v<Abstracts::Component, T>>>
		void RemoveCacheComponent(const boost::shared_ptr<T>& script)
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

							 scene->removeCacheComponentImpl(component, component->GetComponentType());
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
							 const auto& component = std::any_cast<boost::shared_ptr<T>>(params[1]);

							 scene->removeCacheComponentImpl(component, which_component<T>::value);
						 }
						);
			}
		}

		// Add cache script from the object.
		template <typename T, typename ScriptLock = std::enable_if_t<std::is_base_of_v<Script, T>>>
		void AddCacheScript(const boost::shared_ptr<T>& script)
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

							 scene->addCacheScriptImpl(scp, scp->GetScriptType());
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
							 const auto& component = std::any_cast<boost::shared_ptr<T>>(params[1]);

							 scene->addCacheScriptImpl(component, which_script<T>::value);
						 }
						);
			}
		}

		// Remove cache script from the object.
		template <typename T, typename ScriptLock = std::enable_if_t<std::is_base_of_v<Script, T>>>
		void RemoveCacheScript(const boost::shared_ptr<T>& script)
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

							 scene->removeCacheScriptImpl(scp, scp->GetScriptType());
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
							 const auto& scp   = std::any_cast<boost::shared_ptr<T>>(params[1]);

							 scene->removeCacheScriptImpl(scp, which_script<T>::value);
						 }
						);
			}
		}

		template <typename T>
		ConcurrentWeakComVec GetCachedComponents()
		{
			ConcurrentWeakComRootMap::const_accessor acc;

			if (m_cached_components_.find(acc, which_component<T>::value))
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
		ConcurrentWeakScpVec GetCachedScripts()
		{
			ConcurrentWeakScpRootMap::const_accessor acc;

			if (m_cached_scripts_.find(acc, which_script<T>::value))
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

		auto operator[](size_t idx) const
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

	private:
		friend class Managers::SceneManager;

		void AssignLocalIDToObject(const Strong<Abstracts::ObjectBase>& obj);

		// Set the scene and layer to the object, and schedule the object to be added at the next frame.
		void addGameObjectImpl(LayerSizeType layer, const Strong<Abstracts::ObjectBase>& obj);
		// Add cache component from the object.
		void addCacheComponentImpl(const Strong<Abstracts::Component>& component, eComponentType type);
		// Remove cache component from the object.
		void removeCacheComponentImpl(const Strong<Abstracts::Component>& component, eComponentType type);

		// Add cache script from the object.
		void addCacheScriptImpl(const Strong<Script>& script, ScriptSizeType type);
		// Remove cache script from the object.
		void removeCacheScriptImpl(const Strong<Script>& component, ScriptSizeType type);

		// Functions for the next frame.

		// Add the object from the scene finally. this function should be called at the next frame.
		void AddObjectFinalize(LayerSizeType layer, const Strong<Abstracts::ObjectBase>& obj);
		// Remove the object from the scene finally. this function should be called at the next frame.
		void RemoveObjectFinalize(GlobalEntityID id, LayerSizeType layer);
		void initializeFinalize();

		void synchronize(const Weak<Scene>& ptr_scene);

		bool m_b_scene_imgui_open_;
		bool m_b_scene_raytracing_;

		LocalActorID               m_main_camera_local_id_;
		LocalActorID               m_main_actor_local_id_;
		LayerSizeType              m_layer_count_;
		std::vector<Strong<Layer>> m_layers_;

		// Non-serialized
		Weak<Abstracts::ObjectBase> m_observer_;
		Weak<Objects::Camera>                  m_mainCamera_;
		Weak<Abstracts::ObjectBase> m_main_actor_;

		ConcurrentLocalGlobalIDMap                           m_assigned_actor_ids_;
		ConcurrentWeakObjGlobalMap                           m_cached_objects_;
		ConcurrentWeakComRootMap                             m_cached_components_;
		ConcurrentWeakScpRootMap                             m_cached_scripts_;
		Octree<Weak<Abstracts::ObjectBase>, bounding_getter> m_object_position_tree_;

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
