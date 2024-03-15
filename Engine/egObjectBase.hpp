#pragma once
#include "egActor.h"
#include "egCommon.hpp"
#include "egComponent.h"
#include "egResource.h"
#include "egScene.hpp"
#include "egScript.h"

namespace Engine::Abstract
{
  // Abstract base class for objects
  class ObjectBase : public Actor
  {
  public:
    ~ObjectBase() override = default;

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostUpdate(const float& dt) override;

    void OnSerialized() override;
    void OnDeserialized() override;
    void OnImGui() override;

    [[nodiscard]] StrongObjectBase Clone() const;

    template <typename T, typename... Args, typename CLock = std::enable_if_t<std::is_base_of_v<Component, T>>>
    boost::weak_ptr<T> AddComponent(Args&&... args)
    {
      const auto type = which_component<T>::value;

      if (const auto comp = checkComponent(type).lock())
      {
        return boost::static_pointer_cast<T>(comp);
      }

      const auto thisObject = GetSharedPtr<ObjectBase>();

      boost::shared_ptr<T> component =
        boost::make_shared<T>(thisObject, std::forward<Args>(args)...);
      component->Initialize();

      addComponentImpl(component, type);
      addComponentToSceneCache<T>(component);

      return component;
    }

  public:
    template <typename T, typename SLock = std::enable_if_t<std::is_base_of_v<Script, T>>>
    boost::weak_ptr<T> AddScript(const std::string& name = "")
    {
      const auto type = which_script<T>::value;

      if (m_scripts_.contains(which_script<T>::value))
      {
        return boost::static_pointer_cast<T>(m_scripts_[which_script<T>::value]);
      }

      boost::shared_ptr<T> script = boost::make_shared<T>(GetSharedPtr<ObjectBase>());
      script->SetName(name);
      addScriptImpl(script, type);

      return script;
    }

  public:
    template <typename T, typename SLock = std::enable_if_t<std::is_base_of_v<Script, T>>>
    boost::weak_ptr<T> GetScript(const std::string& name = "")
    {
      if (m_scripts_.contains(which_script<T>::value))
      {
        return boost::static_pointer_cast<T>(m_scripts_[which_script<T>::value]);
      }

      return {};
    }

    template <typename T, typename SLock = std::enable_if_t<std::is_base_of_v<Script, T>>>
    void RemoveScript(const std::string& name = "")
    {
      if (m_scripts_.contains(which_script<T>::value))
      {
        auto& scripts = m_scripts_[which_script<T>::value];

        if (name.empty() && !scripts.empty()) { scripts.erase(scripts.begin()); }
        else
        {
          std::erase_if
            (
             scripts, [&name](const StrongScript& script) { return script->GetName() == name; }
            );
        }
      }
    }

    const std::set<WeakComponent, ComponentPriorityComparer>& GetAllComponents();

    template <typename T>
    boost::weak_ptr<T> GetComponent()
    {
      if constexpr (std::is_base_of_v<Component, T>)
      {
        if (!m_components_.contains(which_component<T>::value)) { return {}; }

        const auto& comp = m_components_[which_component<T>::value];

        return boost::static_pointer_cast<T>(comp);
      }

      return {};
    }

    template <typename T, typename CLock = std::enable_if_t<std::is_base_of_v<Component, T>>>
    void RemoveComponent()
    {
      removeComponentFromSceneCache<T>(m_components_[which_component<T>::value]);
      removeComponentImpl(which_component<T>::value);
    }

    template <typename T, typename Lock = std::enable_if_t<std::is_base_of_v<Component, T>>>
    void DispatchComponentEvent(const boost::shared_ptr<T>& other);

    void SetActive(bool active);
    void SetCulled(bool culled);
    void SetImGuiOpen(bool open);

    bool           GetActive() const;
    bool           GetCulled() const;
    bool           GetImGuiOpen() const;
    eDefObjectType GetObjectType() const;

    WeakObject              GetParent() const;
    WeakObject              GetChild(LocalActorID id) const;
    std::vector<WeakObject> GetChildren() const;

    void AddChild(const WeakObject& child);
    bool DetachChild(LocalActorID id);

  protected:
    explicit ObjectBase(eDefObjectType type = DEF_OBJ_T_NONE)
      : Actor(),
        m_parent_id_(g_invalid_id),
        m_type_(type),
        m_active_(true),
        m_culled_(true),
        m_imgui_open_(false) { };

    virtual void OnCollisionEnter(const StrongCollider& other);
    virtual void OnCollisionContinue(const StrongCollider& other);
    virtual void OnCollisionExit(const StrongCollider& other);

  private:
    SERIALIZE_DECL
    friend class Scene;
    friend class Manager::Graphics::ShadowManager;

    // Overridable function for derived object clone behavior.
    [[nodiscard]] virtual StrongObjectBase cloneImpl() const = 0;

    // Check whether the component is already added to the object.
    WeakComponent checkComponent(const eComponentType type);

    // Add component to the scene cache.
    template <typename T, typename CLock = std::enable_if_t<std::is_base_of_v<Component, T>>>
    void addComponentToSceneCache(const boost::shared_ptr<T>& component)
    {
      if (const auto scene = GetScene().lock())
      {
        scene->AddCacheComponent<T>(component);
      }
    }

    template <typename T, typename CLock = std::enable_if_t<std::is_base_of_v<Component, T>>>
    void removeComponentFromSceneCache(const boost::shared_ptr<T>& component)
    {
      if (const auto scene = GetScene().lock())
      {
        scene->RemoveCacheComponent<T>(component);
      }
    }

    // Add pre-existing component to the object.
    WeakComponent addComponent(const StrongComponent& component);

    // Add pre-existing script to the object.
    void addScriptImpl(const StrongScript & script, const eScriptType type);

    // Remove component from the object.
    void removeComponentImpl(eComponentType type);

    // Commit the component to the object.
    void          addComponentImpl(const StrongComponent& component, eComponentType type);

    LocalActorID              m_parent_id_;
    std::vector<LocalActorID> m_children_;
    eDefObjectType            m_type_;
    bool                      m_active_ = true;
    bool                      m_culled_ = true;

    bool m_imgui_open_ = false;
    bool m_imgui_children_open_ = false;
    bool m_imgui_components_open_ = false;

    std::map<eComponentType, StrongComponent> m_components_;

    // Non-serialized
    WeakObject                                         m_parent_;
    std::map<LocalActorID, WeakObject>                 m_children_cache_;
    std::set<LocalComponentID>                         m_assigned_component_ids_;
    std::set<WeakComponent, ComponentPriorityComparer> m_cached_component_;
    std::map<eScriptType, StrongScript>   m_scripts_;
  };
} // namespace Engine::Abstract

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Engine::Abstract::ObjectBase)
BOOST_CLASS_EXPORT_KEY(Engine::Abstract::ObjectBase)
