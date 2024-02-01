#pragma once
#include <ranges>

#include <boost/serialization/export.hpp>
#include "egLayer.h"
#include "egOctree.hpp"
#include "egRenderable.h"
#include "egTaskScheduler.h"

namespace Engine
{
  class Scene : public Abstract::Renderable
  {
  public:
    Scene(const Scene& other) = default;
    ~Scene() override         = default;

    virtual void Initialize_INTERNAL() = 0;
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
    void OnDeserialized() override;
    void OnImGui() override;

    template <typename T, typename ObjLock = std::enable_if_t<std::is_base_of_v<Abstract::Object, T>>>
    boost::weak_ptr<T> CreateGameObject(eLayerType layer)
    {
      // Create object, dynamic allocation from scene due to the access limitation.
      auto obj_t = boost::shared_ptr<T>(new T);
      auto obj   = obj_t->template GetSharedPtr<Abstract::Object>();

      // Set internal information as this scene and layer
      obj->template GetSharedPtr<Abstract::Actor>()->SetScene(GetSharedPtr<Scene>());
      obj->template GetSharedPtr<Abstract::Actor>()->SetLayer(layer);
      AssignLocalIDToObject(obj);

      obj->Initialize();

      // finalize the object registration at the next frame
      GetTaskScheduler().AddTask
        (
         TASK_ADD_OBJ,
         {obj, layer}, // keep the object alive, scene does not own the object yet.
         [this](const std::vector<std::any>& params, const float dt)
         {
           const auto obj   = std::any_cast<StrongObject>(params[0]);
           const auto layer = std::any_cast<eLayerType>(params[1]);

           AddObjectFinalize(layer, obj);
         }
        );


      // yield the currently created object
      return obj_t;
    }

    void RemoveGameObject(GlobalEntityID id, eLayerType layer);

    WeakObject FindGameObject(GlobalEntityID id) const;
    WeakObject FindGameObjectByLocalID(LocalActorID id) const;

    eSceneType           GetType() const;
    ConcurrentWeakObjVec GetGameObjects(eLayerType layer) const;
    WeakCamera           GetMainCamera() const;

    const Octree& GetObjectTree();

    // Add cache component from the object. Type is deduced in compile time.
    template <typename T, typename CompLock = std::enable_if_t<std::is_base_of_v<Abstract::Component, T>>>
    void AddCacheComponent(const boost::shared_ptr<T>& component)
    {
      ConcurrentWeakObjGlobalMap::const_accessor acc;

      if (m_cached_objects_.find(acc, component->GetOwner().lock()->GetID()))
      {
        ConcurrentWeakComRootMap::accessor comp_acc;

        if (m_cached_components_.find(comp_acc, which_component<T>::value))
        {
          comp_acc->second.emplace
            (component->GetID(), component);
        }
        else
        {
          m_cached_components_.insert(comp_acc, which_component<T>::value);
          comp_acc->second.emplace(component->GetID(), component);
        }
      }
    }

    // Add cache component from the object. Type is deduced in runtime.
    void AddCacheComponent(const StrongComponent& component);

    template <typename T, typename CompLock = std::enable_if_t<std::is_base_of_v<Abstract::Component, T>>>
    void RemoveCacheComponent(const boost::shared_ptr<T>& component)
    {
      ConcurrentWeakObjGlobalMap::const_accessor acc;

      if (m_cached_objects_.find(acc, component->GetOwner().lock()->GetID()))
      {
        ConcurrentWeakComRootMap::accessor comp_acc;
        if (m_cached_components_.find(comp_acc, which_component<T>::value))
        {
          comp_acc->second.erase(component->GetID());
        }
      }
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

  protected:
    explicit Scene(eSceneType type);

  private:
    SERIALIZER_ACCESS
    friend class Manager::SceneManager;

    void Synchronize(const StrongScene& scene);
    void OpenLoadPopup(bool& is_load_open);

    void AssignLocalIDToObject(const StrongObject& obj);

    // Functions for the next frame.
    // Add the object from the scene finally. this function should be called at the next frame.
    void AddObjectFinalize(eLayerType layer, const StrongObject& obj);
    // Remove the object from the scene finally. this function should be called at the next frame.
    void RemoveObjectFinalize(GlobalEntityID id, eLayerType layer);
    // Initialize the scene finally. this function should be called at the next frame.
    void InitializeFinalize();

    virtual void AddCustomObject();

    bool m_b_scene_imgui_open_;

    LocalActorID             m_main_camera_local_id_;
    std::vector<StrongLayer> m_layers;
    eSceneType               m_type_;

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
