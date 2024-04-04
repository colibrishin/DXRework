#pragma once
#include <ranges>

#include <boost/serialization/export.hpp>
#include "egLayer.h"
#include "egOctree.hpp"
#include "egRenderable.h"
#include "egTaskScheduler.h"
#include "egComponent.h"
#include "egScript.h"

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

    void           SetMainActor(const LocalActorID id);
    WeakObjectBase GetMainActor() const;

    // Add Object to the scene.
    // If the object is bound to another scene or layer, it will be moved to this scene and layer.
    // Note that the object will be added finally at the next frame.
    template <typename T, typename ObjLock = std::enable_if_t<std::is_base_of_v<Abstract::ObjectBase, T>>>
    void AddGameObject(eLayerType layer, const boost::shared_ptr<T>& obj)
    {
      const auto& downcast = obj->template GetSharedPtr<Abstract::ObjectBase>();
      addGameObjectImpl(layer, downcast);
    }

    // Create Object and add it to the scene.
    // Note that the object will be added finally at the next frame.
    template <typename T, typename... Args, typename ObjLock = std::enable_if_t<std::is_base_of_v<Abstract::ObjectBase, T>>>
    boost::weak_ptr<T> CreateGameObject(eLayerType layer, Args&&... args)
    {
      // Create object, dynamic allocation from scene due to the access limitation.
      const auto& obj_t = boost::shared_ptr<T>(new T(args...));
      const auto& obj   = obj_t->template GetSharedPtr<Abstract::ObjectBase>();

      // Set internal information as this scene and layer, segmenting this process for
      // code re-usability.
      addGameObjectImpl(layer, obj);

      // yield the currently created object
      return obj_t;
    }

    void ChangeLayer(const eLayerType to, const GlobalEntityID id);

    void RemoveGameObject(GlobalEntityID id, eLayerType layer);

    WeakObjectBase FindGameObject(GlobalEntityID id) const;
    WeakObjectBase FindGameObjectByLocalID(LocalActorID id) const;

    ConcurrentWeakObjVec GetGameObjects(eLayerType layer) const;
    WeakCamera           GetMainCamera() const;

    const Octree& GetObjectTree();

    // Add cache component from the object.
    template <typename T, typename CompLock = std::enable_if_t<std::is_base_of_v<Abstract::Component, T>>>
    void AddCacheComponent(const boost::shared_ptr<T>& component)
    {
      // If the component cannot be deduced, go with runtime.
      if constexpr (std::is_same_v<Abstract::Component, T>)
      {
        GetTaskScheduler().AddTask
          (
           TASK_CACHE_COMPONENT,
           {GetSharedPtr<Scene>(), component},
           [](const std::vector<std::any>& params, const float)
           {
             const auto& scene = std::any_cast<StrongScene>(params[0]);
             const auto& component = std::any_cast<StrongComponent>(params[1]);

             scene->addCacheComponentImpl(component, component->GetComponentType());
           }
          );
      }
      else
      {
        GetTaskScheduler().AddTask
          (
           TASK_CACHE_COMPONENT,
           {GetSharedPtr<Scene>(), component},
           [](const std::vector<std::any>& params, const float)
           {
             const auto& scene = std::any_cast<StrongScene>(params[0]);
             const auto& component = std::any_cast<boost::shared_ptr<T>>(params[1]);

             scene->addCacheComponentImpl(component, which_component<T>::value);
           }
          );
      }
    }

    // Remove cache component from the object.
    template <typename T, typename CompLock = std::enable_if_t<std::is_base_of_v<Abstract::Component, T>>>
    void RemoveCacheComponent(const boost::shared_ptr<T>& script)
    {
      // If the component cannot be deduced, go with runtime.
      if constexpr (std::is_same_v<Abstract::Component, T>)
      {
        GetTaskScheduler().AddTask
          (
           TASK_UNCACHE_COMPONENT,
           {GetSharedPtr<Scene>(), script},
           [](const std::vector<std::any>& params, const float)
           {
             const auto& scene = std::any_cast<StrongScene>(params[0]);
             const auto& component = std::any_cast<StrongComponent>(params[1]);

             scene->removeCacheComponentImpl(component, component->GetComponentType());
           }
          );
      }
      else
      {
        GetTaskScheduler().AddTask
          (
           TASK_UNCACHE_COMPONENT,
           {GetSharedPtr<Scene>(), script},
           [](const std::vector<std::any>& params, const float)
           {
             const auto& scene = std::any_cast<StrongScene>(params[0]);
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
        GetTaskScheduler().AddTask
          (
           TASK_CACHE_SCRIPT,
           {GetSharedPtr<Scene>(), script},
           [](const std::vector<std::any>& params, const float)
           {
             const auto& scene = std::any_cast<StrongScene>(params[0]);
             const auto& scp = std::any_cast<StrongScript>(params[1]);

             scene->addCacheScriptImpl(scp, scp->GetScriptType());
           }
          );
      }
      else
      {
        GetTaskScheduler().AddTask
          (
           TASK_CACHE_COMPONENT,
           {GetSharedPtr<Scene>(), script},
           [](const std::vector<std::any>& params, const float)
           {
             const auto& scene = std::any_cast<StrongScene>(params[0]);
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
        GetTaskScheduler().AddTask
          (
           TASK_UNCACHE_SCRIPT,
           {GetSharedPtr<Scene>(), script},
           [](const std::vector<std::any>& params, const float)
           {
             const auto& scene = std::any_cast<StrongScene>(params[0]);
             const auto& scp = std::any_cast<StrongScript>(params[1]);

             scene->removeCacheScriptImpl(scp, scp->GetScriptType());
           }
          );
      }
      else
      {
        GetTaskScheduler().AddTask
          (
           TASK_UNCACHE_SCRIPT,
           {GetSharedPtr<Scene>(), script},
           [](const std::vector<std::any>& params, const float)
           {
             const auto& scene = std::any_cast<StrongScene>(params[0]);
             const auto& scp = std::any_cast<boost::shared_ptr<T>>(params[1]);

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

    auto operator[](size_t idx) const { return m_layers[idx]; }

    auto begin() noexcept { return m_layers.begin(); }

    auto end() noexcept { return m_layers.end(); }

    auto begin() const noexcept { return m_layers.begin(); }

    auto end() const noexcept { return m_layers.end(); }

    auto cbegin() const noexcept { return m_layers.cbegin(); }

    auto cend() const noexcept { return m_layers.cend(); }

  private:
    SERIALIZE_DECL
    friend class Manager::SceneManager;

    void AssignLocalIDToObject(const StrongObjectBase& obj);

    // Set the scene and layer to the object, and schedule the object to be added at the next frame.
    void addGameObjectImpl(eLayerType layer, const StrongObjectBase& obj);
    // Add cache component from the object.
    void addCacheComponentImpl(const StrongComponent& component, const eComponentType type);
    // Remove cache component from the object.
    void removeCacheComponentImpl(const StrongComponent& component, const eComponentType type);

    // Add cache script from the object.
    void addCacheScriptImpl(const StrongScript& script, const eScriptType type);
    // Remove cache script from the object.
    void removeCacheScriptImpl(const StrongScript& component, const eScriptType type);

    // Functions for the next frame.

    // Add the object from the scene finally. this function should be called at the next frame.
    void AddObjectFinalize(eLayerType layer, const StrongObjectBase& obj);
    // Remove the object from the scene finally. this function should be called at the next frame.
    void RemoveObjectFinalize(GlobalEntityID id, eLayerType layer);
    void initializeFinalize();

    void synchronize(const WeakScene& ptr_scene);

    bool m_b_scene_imgui_open_;

    LocalActorID             m_main_camera_local_id_;
    LocalActorID             m_main_actor_local_id_;
    std::vector<StrongLayer> m_layers;

    // Non-serialized
    WeakObjectBase m_observer_;
    WeakCamera m_mainCamera_;
    WeakObjectBase m_main_actor_;

    ConcurrentLocalGlobalIDMap m_assigned_actor_ids_;
    ConcurrentWeakObjGlobalMap m_cached_objects_;
    ConcurrentWeakComRootMap   m_cached_components_;
    ConcurrentWeakScpRootMap   m_cached_scripts_;
    Octree                     m_object_position_tree_;
  };
} // namespace Engine

BOOST_CLASS_EXPORT_KEY(Engine::Scene)
