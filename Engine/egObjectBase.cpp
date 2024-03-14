#include "pch.h"

#include "egObject.hpp"
#include "egAnimator.h"
#include "egBaseCollider.hpp"
#include "egCollision.h"
#include "egComponent.h"
#include "egManagerHelper.hpp"
#include "egMesh.h"
#include "egModelRenderer.h"
#include "egParticleRenderer.h"
#include "egRigidbody.h"
#include "egSoundPlayer.h"
#include "egTransform.h"

SERIALIZE_IMPL
(
 Engine::Abstract::ObjectBase,
 _ARTAG(_BSTSUPER(Actor))
 _ARTAG(m_parent_id_)
 _ARTAG(m_children_)
 _ARTAG(m_type_)
 _ARTAG(m_active_)
 _ARTAG(m_culled_)
 _ARTAG(m_components_)
 _ARTAG(m_scripts_)
)

namespace Engine::Abstract
{
  template void ObjectBase::DispatchComponentEvent(const StrongCollider& other);

  template <typename T, typename Lock>
  void ObjectBase::DispatchComponentEvent(const boost::shared_ptr<T>& other)
  {
    if constexpr (std::is_base_of_v<Component, T>)
    {
      if constexpr (std::is_same_v<Components::Collider, T>)
      {
        const auto rhs_owner = other->GetOwner().lock();

        const auto collision_check = GetCollisionDetector().IsCollided
                                     (
                                      GetID(), rhs_owner->GetID()
                                     ) || GetCollisionDetector().IsCollided
                                     (
                                      rhs_owner->GetID(), GetID()
                                     );
        const auto collision_frame = GetCollisionDetector().IsCollidedInFrame
                                     (
                                      GetID(), rhs_owner->GetID()
                                     ) || GetCollisionDetector().IsCollidedInFrame
                                     (
                                      rhs_owner->GetID(), GetID()
                                     );

        if (collision_frame)
        {
          GetDebugger().Log
            (
             "Collision detected between " + GetName() + " and " + rhs_owner->GetName()
            );

          OnCollisionEnter(other);
        }
        else if (collision_check && !collision_frame) { OnCollisionContinue(other); }
        else if (!collision_check && !collision_frame)
        {
          GetDebugger().Log
            (
             "Collision exit between " + GetName() + " and " + rhs_owner->GetName()
            );
          OnCollisionExit(other);
        }
      }
    }
  }

  void ObjectBase::SetActive(bool active) { m_active_ = active; }

  void ObjectBase::SetCulled(bool culled) { m_culled_ = culled; }

  void ObjectBase::SetImGuiOpen(bool open) { m_imgui_open_ = open; }

  bool ObjectBase::GetActive() const { return m_active_; }

  bool ObjectBase::GetCulled() const { return m_culled_; }

  bool ObjectBase::GetImGuiOpen() const { return m_imgui_open_; }

  eDefObjectType ObjectBase::GetObjectType() const { return m_type_; }

  WeakObject ObjectBase::GetParent() const
  {
    INVALID_ID_CHECK_WEAK_RETURN(m_parent_id_)
    return m_parent_;
  }

  WeakObject ObjectBase::GetChild(const LocalActorID id) const
  {
    INVALID_ID_CHECK_WEAK_RETURN(id)

    if (m_children_cache_.contains(id)) { return m_children_cache_.at(id); }

    return {};
  }

  std::vector<WeakObject> ObjectBase::GetChildren() const
  {
    std::vector<WeakObject> out;

    for (const auto& child : m_children_cache_ | std::views::values)
    {
      if (const auto locked = child.lock()) { out.push_back(locked); }
    }

    return out;
  }

  void ObjectBase::AddChild(const WeakObject& p_child)
  {
    if (const auto child = p_child.lock())
    {
      m_children_.push_back(child->GetLocalID());
      m_children_cache_.insert({child->GetLocalID(), child});

      if (child->m_parent_id_ != g_invalid_id) { child->m_parent_.lock()->DetachChild(child->GetLocalID()); }

      child->m_parent_id_ = GetLocalID();
      child->m_parent_    = GetSharedPtr<ObjectBase>();
    }
  }

  bool ObjectBase::DetachChild(const LocalActorID id)
  {
    if (id == g_invalid_id) { return false; }

    if (m_children_cache_.contains(id))
    {
      m_children_cache_.at(id).lock()->m_parent_id_ = g_invalid_id;
      m_children_cache_.at(id).lock()->m_parent_    = {};
      m_children_cache_.erase(id);
      m_children_.erase(std::ranges::find(m_children_, id));

      return true;
    }

    return false;
  }

  void ObjectBase::OnCollisionEnter(const StrongCollider& other)
  {
    if (!GetComponent<Components::Collider>().lock()) { throw std::exception("Object has no collider"); }
  }

  void ObjectBase::OnCollisionContinue(const StrongCollider& other)
  {
    if (!GetComponent<Components::Collider>().lock()) { throw std::exception("Object has no collider"); }
  }

  void ObjectBase::OnCollisionExit(const StrongCollider& other)
  {
    if (!GetComponent<Components::Collider>().lock()) { throw std::exception("Object has no collider"); }
  }

  WeakComponent ObjectBase::checkComponent(const eComponentType type)
  {
    if (m_components_.contains(type))
    {
      return m_components_[type];
    }

    return {};
  }

  WeakComponent ObjectBase::addComponent(const StrongComponent& component)
  {
    const auto type = component->GetComponentType();

    if (const auto comp = checkComponent(type).lock())
    {
      return comp;
    }

    // Change the owner of the component. Since the component is already added to the cache, skipping the uncaching.
    if(const auto prev = component->GetOwner().lock())
    {
      prev->removeComponentImpl(component->GetComponentType());
    }

    component->SetOwner(GetSharedPtr<ObjectBase>());

    // Add the component to the object.
    addComponentImpl(component, type);

    return component;
  }

  void ObjectBase::addScriptImpl(const StrongScript& script, const eScriptType type)
  {
    if (m_scripts_.contains(type))
    {
      return;
    }

    script->SetOwner(GetSharedPtr<ObjectBase>());
    if (!script->IsInitialized())
    {
      script->Initialize();
    }

    m_scripts_.emplace(type, script);
  }

  void ObjectBase::removeComponentImpl(const eComponentType type)
  {
    if (m_components_.contains(type))
    {
      const auto comp = m_components_[type];

      m_assigned_component_ids_.erase(comp->GetLocalID());
      m_cached_component_.erase(comp);
      m_components_.erase(type);
    }
  }

  void ObjectBase::addComponentImpl(const StrongComponent& component, eComponentType type)
  {
    m_components_.emplace(type, component);

    UINT idx = 0;

    while (true)
    {
      if (idx == g_invalid_id)
      {
        throw std::exception("Component ID overflow");
      }

      if (!m_assigned_component_ids_.contains(idx))
      {
        component->SetLocalID(idx);
        m_assigned_component_ids_.insert(idx);
        break;
      }

      idx++;
    }

    m_cached_component_.insert(component);
  }

  void ObjectBase::Render(const float& dt)
  {
    for (const auto& script : m_scripts_ | std::views::values)
    {
      if (!script->GetActive()) { continue; }

      script->Render(dt);
    }

    for (const auto& child : m_children_cache_ | std::views::values)
    {
      if (const auto locked = child.lock())
      {
        if (!locked->GetActive()) { continue; }

        if (locked->GetCulled() && !GetProjectionFrustum().CheckRender(locked)) { continue; }

        locked->Render(dt);
      }
    }
  }

  void ObjectBase::PostRender(const float& dt)
  {
    for (const auto& script : m_scripts_ | std::views::values)
    {
      if (!script->GetActive()) { continue; }

      script->PostRender(dt);
    }

    for (const auto& child : m_children_cache_ | std::views::values)
    {
      if (const auto locked = child.lock())
      {
        if (!locked->GetActive()) { continue; }

        locked->PostRender(dt);
      }
    }
  }

  void ObjectBase::FixedUpdate(const float& dt)
  {
    for (const auto& script : m_scripts_ | std::views::values)
    {
      if (!script->GetActive()) { continue; }

      script->FixedUpdate(dt);
    }

    for (const auto& component : m_components_ | std::views::values)
    {
      if (!component->GetActive()) { continue; }

      component->FixedUpdate(dt);
    }

    for (const auto& child : m_children_cache_ | std::views::values)
    {
      if (const auto locked = child.lock())
      {
        if (!locked->GetActive()) { continue; }

        locked->FixedUpdate(dt);
      }
    }
  }

  void ObjectBase::PostUpdate(const float& dt)
  {
    for (const auto& script : m_scripts_ | std::views::values)
    {
      if (!script->GetActive()) { continue; }

      script->PostUpdate(dt);
    }

    for (const auto& component : m_components_ | std::views::values)
    {
      if (!component->GetActive()) { continue; }

      component->PostUpdate(dt);
    }

    for (const auto& child : m_children_cache_ | std::views::values)
    {
      if (const auto locked = child.lock())
      {
        if (!locked->GetActive()) { continue; }

        locked->PostUpdate(dt);
      }
    }
  }

  void ObjectBase::OnSerialized()
  {
    for (const auto& comp : m_components_ | std::views::values)
    {
      comp->OnSerialized();
    }
  }

  void ObjectBase::OnDeserialized()
  {
    Actor::OnDeserialized();

    for (const auto& comp : m_components_ | std::views::values)
    {
      comp->SetOwner(GetSharedPtr<ObjectBase>());
      comp->OnDeserialized();
      m_assigned_component_ids_.insert(comp->GetLocalID());
      m_cached_component_.insert(comp);
    }

    for (const auto& script : m_scripts_ | std::views::values)
    {
      script->SetOwner(GetSharedPtr<ObjectBase>());
      script->OnDeserialized();
    }
  }

  void ObjectBase::OnImGui()
  {
    const auto id = GetTypeName() + " " + GetName() + "###" + std::to_string(GetID());

    if (ImGui::Begin
      (
       id.c_str(), nullptr,
       ImGuiWindowFlags_AlwaysAutoResize |
       ImGuiWindowFlags_NoCollapse
      ))
    {
      ImGui::BulletText("Object");
      Actor::OnImGui();
      ImGui::Indent(2);
      ImGui::Checkbox("Active", &m_active_);
      ImGui::Checkbox("Culling", &m_culled_);

      if (ImGui::Button("Children"))
      {
        m_imgui_children_open_ = !m_imgui_children_open_;
      }
      ImGui::SameLine();

      if (ImGui::Button("Add Components"))
      {
        m_imgui_components_open_ = !m_imgui_components_open_;
      }

      ImGui::SameLine();

      if (m_imgui_children_open_)
      {
        if (ImGui::Begin("Children", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
          for (const auto& child : m_children_cache_ | std::views::values)
          {
            if (const auto locked = child.lock())
            {
              const auto child_id = locked->GetTypeName() + " " + locked->GetName() + " " + std::to_string(locked->GetID());

              if (ImGui::Selectable(child_id.c_str())) { locked->SetImGuiOpen(!locked->GetImGuiOpen()); }

              if (locked->GetImGuiOpen()) { locked->OnImGui(); }
            }
          }

          ImGui::End();
        }
      }

      if (m_imgui_components_open_)
      {
        if (ImGui::Begin("Add Components", &m_imgui_components_open_, ImGuiChildFlags_AlwaysAutoResize))
        {
          if (ImGui::Button("Transform"))
          {
            AddComponent<Components::Transform>();
          }

          if (ImGui::Button("Animator"))
          {
            AddComponent<Components::Animator>();
          }

          if (ImGui::Button("ModelRenderer"))
          {
            AddComponent<Components::ModelRenderer>();
          }

          if (ImGui::Button("ParticleRenderer"))
          {
            AddComponent<Components::ParticleRenderer>();
          }

          if (ImGui::Button("Collider"))
          {
            AddComponent<Components::Collider>();
          }

          if (ImGui::Button("Rigidbody"))
          {
            AddComponent<Components::Rigidbody>();
          }

          if (ImGui::Button("SoundPlayer"))
          {
            AddComponent<Components::SoundPlayer>();
          }

          for (const auto& [script_name, script_func] : Script::GetScriptFactory())
          {
            if (ImGui::Button(script_name.c_str()))
            {
              const auto& script = script_func(GetSharedPtr<ObjectBase>());
              addScriptImpl(script, script->GetScriptType());
            }
          }

          ImGui::Separator();
          ImGui::End();
        }
      }

      if (ImGui::TreeNode("Scripts"))
      {
        for (const auto& script : m_scripts_ | std::views::values)
        {
          if (ImGui::TreeNode(script->GetTypeName().c_str()))
          {
            script->OnImGui();
            ImGui::TreePop();
            ImGui::Spacing();
          }
        }

        ImGui::TreePop();
        ImGui::Spacing();
      }

      if (ImGui::TreeNode("Components"))
      {
        for (const auto& comp : m_components_ | std::views::values)
        {
          if (ImGui::TreeNode(comp->GetTypeName().c_str()))
          {
            comp->OnImGui();
            ImGui::TreePop();
            ImGui::Spacing();
          }
        }

        ImGui::TreePop();
        ImGui::Spacing();
      }

      ImGui::Unindent(2);
      ImGui::End();
    }
  }

  StrongObjectBase ObjectBase::Clone() const
  {
    const auto& cloned = cloneImpl();

    // Flip the ImGui flags just in case.
    cloned->m_imgui_children_open_ = false;
    cloned->m_imgui_components_open_ = false;
    cloned->m_imgui_open_ = false;

    // Clone components and scripts
    cloned->m_components_.clear();
    cloned->m_scripts_.clear();

    // Erase the copied pointers that indicates original object.
    cloned->m_assigned_component_ids_.clear();
    cloned->m_cached_component_.clear();
    cloned->m_scripts_.clear();
    cloned->m_components_.clear();

    // Copy components
    for (const auto& comp : m_components_ | std::views::values)
    {
      const auto& cloned_comp = comp->Clone(cloned);
      cloned->addComponent(cloned_comp);
    }

    // Copy scripts
    for (const auto& script : m_scripts_ | std::views::values)
    {
      const auto& cloned_script = script->Clone(cloned);
      cloned->addScriptImpl(cloned_script, cloned_script->GetScriptType());
    }

    // Keep intact with the parent.

    // Clone children
    cloned->m_children_.clear();

    for (const auto& child : m_children_cache_ | std::views::values)
    {
      if (const auto locked = child.lock())
      {
        const auto cloned_child = locked->Clone();
        cloned->AddChild(cloned_child);
      }
    }

    // Add to the scene finally.
    GetScene().lock()->AddGameObject(GetLayer(), cloned);

    // Keep in mind that this object is yielded, not added to the scene yet.
    return cloned;
  }

  const std::set<WeakComponent, ComponentPriorityComparer>& ObjectBase::GetAllComponents() { return m_cached_component_; }

  void ObjectBase::PreUpdate(const float& dt)
  {
    for (const auto& script : m_scripts_ | std::views::values)
    {
      if (!script->GetActive()) { continue; }

      script->PreUpdate(dt);
    }

    for (const auto& component : m_components_ | std::views::values)
    {
      if (!component->GetActive()) { continue; }

      component->PreUpdate(dt);
    }

    for (const auto& child : m_children_cache_ | std::views::values)
    {
      if (const auto locked = child.lock())
      {
        if (!locked->GetActive()) { continue; }

        locked->PreUpdate(dt);
      }
    }
  }

  void ObjectBase::PreRender(const float& dt)
  {
    for (const auto& script : m_scripts_ | std::views::values)
    {
      if (!script->GetActive()) { continue; }

      script->PreRender(dt);
    }

    for (const auto& child : m_children_cache_ | std::views::values)
    {
      if (const auto locked = child.lock())
      {
        if (!locked->GetActive()) { continue; }

        locked->PreRender(dt);
      }
    }
  }

  void ObjectBase::Update(const float& dt)
  {
    for (const auto& script : m_scripts_ | std::views::values)
    {
      if (!script->GetActive()) { continue; }

      script->Update(dt);
    }

    for (const auto& component : m_components_ | std::views::values)
    {
      if (!component->GetActive()) { continue; }

      component->Update(dt);
    }

    for (const auto& child : m_children_cache_ | std::views::values)
    {
      if (const auto locked = child.lock())
      {
        if (!locked->GetActive()) { continue; }

        locked->Update(dt);
      }
    }
  }
} // namespace Engine::Abstract
