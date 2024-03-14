#pragma once
#include <ranges>

#include <boost/serialization/export.hpp>
#include "egLayer.h"
#include "egOctree.hpp"
#include "egRenderable.h"
#include "egTaskScheduler.h"
#include "egComponent.h"

namespace Engine
{
  class Scene : public Abstract::Renderable
  {
  public:
    Scene();
    Scene(const Scene& other) = default;
    ~Scene() override         = default;

    void         DisableControllers();
    void         AddObserver();
    void         Initialize() final;

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PreRender(const float& dt) override;
    void Save();
    void Render(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostRender(const float& dt) override;
    void PostUpdate(const float& dt) override;

    void OnSerialized() override;
    void OnDeserialized() override;
    void OnImGui() override;

    // Add Object to the scene. Type is deduced in compile time.
    // If the object is bound to another scene or layer, it will be moved to this scene and layer.
    template <typename T, typename ObjLock = std::enable_if_t<std::is_base_of_v<Abstract::Object, T>>>
    void AddGameObject(eLayerType layer, const boost::shared_ptr<T>& obj)
    {
      const auto& downcast = obj->template GetSharedPtr<Abstract::Object>();
      addGameObjectImpl(layer, downcast);
    }

    template <typename T, typename... Args, typename ObjLock = std::enable_if_t<std::is_base_of_v<Abstract::Object, T>>>
    boost::weak_ptr<T> CreateGameObject(eLayerType layer, Args&&... args)
    {
      // Create object, dynamic allocation from scene due to the access limitation.
      const auto& obj_t = boost::shared_ptr<T>(new T(args...));
      const auto& obj   = obj_t->template GetSharedPtr<Abstract::Object>();

      // Set internal information as this scene and layer, segmenting this process for
      // code re-usability.
      addGameObjectImpl(layer, obj);

      // yield the currently created object
      return obj_t;
    }

    void RemoveGameObject(GlobalEntityID id, eLayerType layer);

    WeakObject FindGameObject(GlobalEntityID id) const;
    WeakObject FindGameObjectByLocalID(LocalActorID id) const;

    ConcurrentWeakObjVec GetGameObjects(eLayerType layer) const;
    WeakCamera           GetMainCamera() const;

    const Octree& GetObjectTree();

    // Add cache component from the object. Type is deduced in compile time.
    // If component type is generic, use the overloaded runtime version.
    template <typename T, typename CompLock = std::enable_if_t<std::is_base_of_v<Abstract::Component, T>>>
    void AddCacheComponent(const boost::weak_ptr<T>& component)
    {
      GetTaskScheduler().AddTask
        (
         TASK_CACHE_COMPONENT,
         {component},
         [this](const std::vector<std::any>& params, const float)
         {
           const auto& component = std::any_cast<boost::weak_ptr<T>>(params[0]).lock();

           addCacheComponentImpl(component, which_component<T>::value);
         }
        );
    }

    // Add cache component from the object. Type is deduced in runtime.
    void AddCacheComponent(const WeakComponent& component)
    {
      GetTaskScheduler().AddTask
        (
         TASK_CACHE_COMPONENT,
         {component},
         [this](const std::vector<std::any>& params, const float)
         {
           const auto& component = std::any_cast<WeakComponent>(params[0]).lock();

           addCacheComponentImpl(component, component->GetComponentType());
         }
        );
    }

    // Remove cache component from the object. Type is deduced in compile time.
    template <typename T, typename CompLock = std::enable_if_t<std::is_base_of_v<Abstract::Component, T>>>
    void RemoveCacheComponent(const boost::shared_ptr<T>& component)
    {
      GetTaskScheduler().AddTask
        (
         TASK_UNCACHE_COMPONENT,
         {component},
         [this](const std::vector<std::any>& params, const float)
         {
           const auto& component = std::any_cast<boost::shared_ptr<T>>(params[0]);

           removeCacheComponentImpl(component, which_component<T>::value);
         }
        );
    }

    template <typename T>
    ConcurrentWeakComVec GetCachedComponents()
    {
      ConcurrentWeakComRootMap::const_accessor acc;

      if (m_cached_components_.find(acc, which_component<T>::value))
      {
        ConcurrentWeakComVec result;

        for (const auto& comp : acc->second | std::views::values) { result.push_back(comp); }

        return result;
      }

      return {};
    }

    auto operator[](size_t idx) const { return m_layers[idx]; }

    auto begin() noexcept { return m_layers.begin(); }

    auto end() noexcept { return m_layers.end(); }

    auto begin() const noexcept { return m_layers.begin(); }

    auto end() const noexcept { return m_layers.end(); }

    auto cbegin() const noexcept { return m_layers.cbegin(); }

    auto cend() const noexcept { return m_layers.cend(); }

  private:
    SERIALIZER_ACCESS
    friend class Manager::SceneManager;

    void AssignLocalIDToObject(const StrongObject& obj);

    // Set the scene and layer to the object, and schedule the object to be added at the next frame.
    void addGameObjectImpl(eLayerType layer, const StrongObject& obj);
    // Add cache component from the object.
    void addCacheComponentImpl(const StrongComponent& component, const eComponentType type);
    // Remove cache component from the object.
    void removeCacheComponentImpl(const StrongComponent& component, const eComponentType type);

    // Functions for the next frame.

    // Add the object from the scene finally. this function should be called at the next frame.
    void AddObjectFinalize(eLayerType layer, const StrongObject& obj);
    // Remove the object from the scene finally. this function should be called at the next frame.
    void RemoveObjectFinalize(GlobalEntityID id, eLayerType layer);
    void initializeFinalize();

    void synchronize(const WeakScene& ptr_scene);

    bool m_b_scene_imgui_open_;

    LocalActorID             m_main_camera_local_id_;
    std::vector<StrongLayer> m_layers;

    // Non-serialized
    WeakObject m_observer_;
    WeakCamera m_mainCamera_;

    ConcurrentLocalGlobalIDMap m_assigned_actor_ids_;
    ConcurrentWeakObjGlobalMap m_cached_objects_;
    ConcurrentWeakComRootMap   m_cached_components_;
    Octree                     m_object_position_tree_;
  };
} // namespace Engine

BOOST_CLASS_EXPORT_KEY(Engine::Scene)
