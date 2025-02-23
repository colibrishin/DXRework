#pragma once
#include <any>
#include <ranges>
#include <boost/serialization/export.hpp>

#include "Source/Runtime/Core/Component/Public/Component.h"
#include "Source/Runtime/CoreEntity/Public/Renderable.h"
#include "Source/Runtime/Core/BoundingGetter/Public/BoundingGetter.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "TaskScheduler.h"
#include "Source/Runtime/Core/Octree/Public/Octree.hpp"
#include "Source/Runtime/Core/Delegation/Public/Delegation.hpp"
#include "SingletonSpinLock/Public/SingletonSpinLock.h"


#include "Scene.generated.h"

#ifdef PHYSX_ENABLED
namespace physx
{
	class PxScene;
}
#endif

DEFINE_DELEGATE( OnObjectAdded, Engine::Weak<Engine::Abstracts::ObjectBase> );
DEFINE_DELEGATE( OnObjectRemoved, Engine::Weak<Engine::Abstracts::ObjectBase> );

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
		~Scene() override;

		void AddObserver();
		void Initialize() override;

		virtual void BeginPlay( const float dt );
        virtual void EndPlay( const float dt );
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostRender(const float dt) override;
		void PostUpdate(const float dt) override;

#if WITH_EDITOR
		void OnUIUpdate(UIContext* const parent, const float dt) override;
#endif

		void OnSerialized() override;
		void OnDeserialized() override;

		void                        SetMainActor(LocalActorID id);
		Weak<Abstracts::ObjectBase> GetMainActor() const;

		// Add Object to the scene.
		// If the object is bound to another scene or layer, it will be moved to this scene and layer.
		// Note that the object will be added finally at the next frame.
		template <typename T, typename ObjLock = std::enable_if_t<std::is_base_of_v<Abstracts::ObjectBase, T>>>
		void AddGameObject(const LayerSizeType layer, const Strong<T>& obj)
		{
			const auto& downcast = obj->template GetSharedPtr<Abstracts::ObjectBase>();
			addGameObjectImpl(layer, downcast);
		}

		// Create Object and add it to the scene.
		// Note that the object will be added finally at the next frame.
		template <typename T, typename... Args> requires std::is_base_of_v<Abstracts::ObjectBase, T>
		Weak<T> CreateGameObject(const LayerSizeType layer, Args&&... args)
		{
			// Create object, dynamic allocation from scene due to the access limitation.
			const auto& obj_t = Strong<T>(new T(std::forward<Args>(args)...));
			const auto& obj   = obj_t->template GetSharedPtr<Abstracts::ObjectBase>();

			// Set internal information as this scene and layer, segmenting this process for
			// code re-usability.
			addGameObjectImpl(layer, obj);

			// yield the currently created object
			return obj_t;
		}

		void ChangeLayer(const LayerSizeType to, const GlobalEntityID id);

		void RemoveGameObject(const GlobalEntityID id, const LayerSizeType layer);

		Weak<Abstracts::ObjectBase> FindGameObject(GlobalEntityID id);
		Weak<Abstracts::ObjectBase> FindGameObjectByLocalID(LocalActorID id);

		ConcurrentWeakObjVec  GetGameObjectsConcurrent(LayerSizeType layer) const;
		WeakObjVec  GetGameObjects(LayerSizeType layer) const;
		Weak<Objects::Camera> GetMainCamera() const;

		const Octree& GetObjectTree();
		const Octree& GetCollisionTree();

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

		const bool (& GetCollisionMask() const)[RESERVED_LAYER_MAX + CFG_LAYER_COUNT][RESERVED_LAYER_MAX + CFG_LAYER_COUNT];
		void          UpdateCollisionMask(const bool collision_mask[RESERVED_LAYER_MAX + CFG_LAYER_COUNT][RESERVED_LAYER_MAX + CFG_LAYER_COUNT]);
        const std::vector<Weak<Objects::Light>>& GetLights() const;

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

	    [[nodiscard]] Strong<Scene> Clone()
		{ 
		    return cloneImpl();
		}

	    Scene& operator=(const Scene&) = delete;
	    
	protected:
	    Scene(const Scene& other) = default;

	    // Mark the scene as initialize without initialization. (e.g., clone)
	    void initializeForce();
	    virtual Strong<Scene> cloneImpl();
	    virtual void initializeImpl();
	    
	private:
		friend class Managers::SceneManager;

		void AssignLocalIDToObject(const Strong<Abstracts::ObjectBase>& obj);

		// Set the scene and layer to the object, and schedule the object to be added at the next frame.
        void addGameObjectImpl( 
			const LayerSizeType layer, 
			const Strong<Abstracts::ObjectBase> &obj, 
			bool assign_local_id = true );
		
		// Add cache component from the object.
		void addCacheComponentImpl(const Strong<Abstracts::Component>& component, ComponentType type);
		// Remove cache component from the object.
		void removeCacheComponentImpl(const Strong<Abstracts::Component>& component, ComponentType type);
		
		// Functions for the next frame.
		// Add the object from the scene finally. this function should be called at the next frame.
		void AddObjectFinalize(const LayerSizeType layer, const Strong<Abstracts::ObjectBase>& obj);
		// Remove the object from the scene finally. this function should be called at the next frame.
		void RemoveObjectFinalize(const GlobalEntityID id, const LayerSizeType layer);
		void initializeFinalize();

		void synchronize(const Weak<Scene>& ptr_scene);
        void deepCopy( const Weak<Scene> &other );

		EPROPERTY()
		LocalActorID m_main_camera_local_id_;

		EPROPERTY()
		LocalActorID m_main_actor_local_id_;

		EPROPERTY()
		std::vector<Strong<Layer>> m_layers_;

		EPROPERTY()
		bool m_collision_mask_[RESERVED_LAYER_MAX + CFG_LAYER_COUNT][RESERVED_LAYER_MAX + CFG_LAYER_COUNT]{};

#if WITH_EDITOR
		std::string m_layer_list_box_name_;
#endif

		Weak<Abstracts::ObjectBase> m_observer_;
		Weak<Objects::Camera>       m_main_camera_;
		Weak<Abstracts::ObjectBase> m_main_actor_;
        std::vector<Weak<Objects::Light>> m_lights_;

		LocalGlobalIDMap m_assigned_actor_ids_;

		WeakObjGlobalMap m_cached_objects_;
        WeakComRootMap   m_cached_components_;

        SpinLockTicket m_object_lock_;
		SpinLockTicket m_component_lock_;

		ConcurrentWeakObjGlobalMap m_concurrent_cached_objects_;
        ConcurrentWeakComRootMap   m_concurrent_cached_components_;
		
		Octree m_object_position_tree_;
		Octree m_object_collision_tree_;

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
