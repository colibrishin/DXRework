#include "pch.h"

#include "egObject.hpp"
#include "egCollision.h"
#include "egManagerHelper.hpp"
#include "egMesh.h"
#include "egRigidbody.h"
#include "egTransform.h"
#include "egComponent.h"
#include "egBaseCollider.hpp"

SERIALIZER_ACCESS_IMPL(
                       Engine::Abstract::Object,
                       _ARTAG(_BSTSUPER(Actor)) 
                       _ARTAG(m_parent_id_)
                       _ARTAG(m_children_)
                       _ARTAG(m_type_)
                       _ARTAG(m_active_)
                       _ARTAG(m_culled_)
                       _ARTAG(m_components_))

namespace Engine::Abstract
{
    template void Object::DispatchComponentEvent(const StrongCollider& other);

    template <typename T, typename Lock>
    void Object::DispatchComponentEvent(const boost::shared_ptr<T>& other)
    {
        if constexpr (std::is_base_of_v<Component, T>)
        {
            if constexpr (std::is_same_v<Engine::Components::Collider, T>)
            {
                const auto rhs_owner = other->GetOwner().lock();

                const auto speculation_check = GetCollisionDetector().IsSpeculated(
                    GetID(), rhs_owner->GetID()) || GetCollisionDetector().IsSpeculated(
                    rhs_owner->GetID(), GetID());
                const auto collision_check = GetCollisionDetector().IsCollided(
                    GetID(), rhs_owner->GetID()) || GetCollisionDetector().IsCollided(
                    rhs_owner->GetID(), GetID());
                const auto collision_frame = GetCollisionDetector().IsCollidedInFrame(
                    GetID(), rhs_owner->GetID()) || GetCollisionDetector().IsCollidedInFrame(
                    rhs_owner->GetID(), GetID());

                if (collision_frame || speculation_check)
                {
                    GetDebugger().Log(
                                      "Collision detected between " + GetName() + " and " + rhs_owner->GetName());

                    OnCollisionEnter(other);
                }
                else if (collision_check && !collision_frame)
                {
                    OnCollisionContinue(other);
                }
                else if (!collision_check && !collision_frame)
                {
                    GetDebugger().Log(
                                      "Collision exit between " + GetName() + " and " + rhs_owner->GetName());
                    OnCollisionExit(other);
                }
            }
        }
    }

    void Object::SetActive(bool active)
    {
        m_active_ = active;
    }

    void Object::SetCulled(bool culled)
    {
        m_culled_ = culled;
    }

    void Object::SetImGuiOpen(bool open)
    {
        m_imgui_open_ = open;
    }

    bool Object::GetActive() const
    {
        return m_active_;
    }

    bool Object::GetCulled() const
    {
        return m_culled_;
    }

    bool Object::GetImGuiOpen() const
    {
        return m_imgui_open_;
    }

    eDefObjectType Object::GetObjectType() const
    {
        return m_type_;
    }

    WeakObject Object::GetParent() const
    {
        INVALID_ID_CHECK_WEAK_RETURN(m_parent_id_)
        return m_parent_;
    }

    WeakObject Object::GetChild(const LocalActorID id) const
    {
        INVALID_ID_CHECK_WEAK_RETURN(id)

        if (m_children_cache_.contains(id))
        {
            return m_children_cache_.at(id);
        }

        return {};
    }

    std::vector<WeakObject> Object::GetChildren() const
    {
        std::vector<WeakObject> out;

        for (const auto& child : m_children_cache_ | std::views::values)
        {
            if (const auto locked = child.lock())
            {
                out.push_back(locked);
            }
        }

        return out;
    }

    void Object::AddChild(const WeakObject& p_child)
    {
        if (const auto child = p_child.lock())
        {
            m_children_.push_back(child->GetLocalID());
            m_children_cache_.insert({ child->GetLocalID(), child });

            if (child->m_parent_id_ != g_invalid_id)
            {
                child->m_parent_.lock()->DetachChild(child->GetLocalID());
            }

            child->m_parent_id_ = GetLocalID();
            child->m_parent_ = GetSharedPtr<Object>();
        }
    }

    bool Object::DetachChild(const LocalActorID id)
    {
        if (id == g_invalid_id)
        {
            return false;
        }

        if (m_children_cache_.contains(id))
        {
            m_children_cache_.at(id).lock()->m_parent_id_ = g_invalid_id;
            m_children_cache_.at(id).lock()->m_parent_ = {};
            m_children_cache_.erase(id);
            m_children_.erase(std::ranges::find(m_children_, id));
            
            return true;
        }

        return false;
    }

    void Object::OnCollisionEnter(const StrongCollider& other)
    {
        if (!GetComponent<Engine::Components::Collider>().lock())
        {
            throw std::exception("Object has no collider");
        }
    }

    void Object::OnCollisionContinue(const StrongCollider& other)
    {
        if (!GetComponent<Engine::Components::Collider>().lock())
        {
            throw std::exception("Object has no collider");
        }
    }

    void Object::OnCollisionExit(const StrongCollider& other)
    {
        if (!GetComponent<Engine::Components::Collider>().lock())
        {
            throw std::exception("Object has no collider");
        }
    }

    void Object::Render(const float& dt)
    {
        if (m_culled_ && !GetProjectionFrustum().CheckRender(GetWeakPtr<Object>()))
        {
            return;
        }

        for (const auto& child : m_children_cache_ | std::views::values)
        {
            if (const auto locked = child.lock())
            {
                if (!locked->GetActive())
                {
                    continue;
                }

                locked->Render(dt);
            }
        }
    }

    void Object::PostRender(const float& dt)
    {
        for (const auto& child : m_children_cache_ | std::views::values)
        {
            if (const auto locked = child.lock())
            {
                if (!locked->GetActive())
                {
                    continue;
                }

                locked->PostRender(dt);
            }
        }
    }

    void Object::FixedUpdate(const float& dt)
    {
        for (const auto& component : m_components_ | std::views::values)
        {
            if (!component->GetActive())
            {
                continue;
            }

            component->FixedUpdate(dt);
        }

        for (const auto& child : m_children_cache_ | std::views::values)
        {
            if (const auto locked = child.lock())
            {
                if (!locked->GetActive())
                {
                    continue;
                }

                locked->FixedUpdate(dt);
            }
        }
    }

    void Object::PostUpdate(const float& dt)
    {
        for (const auto& component : m_components_ | std::views::values)
        {
            if (!component->GetActive())
            {
                continue;
            }

            component->PostUpdate(dt);
        }

        for (const auto& child : m_children_cache_ | std::views::values)
        {
            if (const auto locked = child.lock())
            {
                if (!locked->GetActive())
                {
                    continue;
                }

                locked->PostUpdate(dt);
            }
        }
    }

    void Object::OnDeserialized()
    {
        Actor::OnDeserialized();

        for (const auto& comp : m_components_ | std::views::values)
        {
            comp->OnDeserialized();
            comp->SetOwner(GetSharedPtr<Object>());
            m_assigned_component_ids_.insert(comp->GetLocalID());
            m_cached_component_.insert(comp);
        }
    }

    void Object::OnImGui()
    {
        const auto id = GetTypeName() + " " + GetName() + " " + std::to_string(GetID());

        if (ImGui::Begin(
                         id.c_str(), nullptr,
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoCollapse))
        {
            ImGui::BulletText("Object");
            Actor::OnImGui();
            ImGui::Indent(2);
            ImGui::Checkbox("Active", &m_active_);
            ImGui::Checkbox("Culling", &m_culled_);

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

    const std::set<WeakComponent, ComponentPriorityComparer>& Object::GetAllComponents()
    {
        return m_cached_component_;
    }

    void Object::PreUpdate(const float& dt)
    {
        for (const auto& component : m_components_ | std::views::values)
        {
            if (!component->GetActive())
            {
                continue;
            }

            component->PreUpdate(dt);
        }

        for (const auto& child : m_children_cache_ | std::views::values)
        {
            if (const auto locked = child.lock())
            {
                if (!locked->GetActive())
                {
                    continue;
                }

                locked->PreUpdate(dt);
            }
        }
    }

    void Object::PreRender(const float& dt)
    {
        for (const auto& child : m_children_cache_ | std::views::values)
        {
            if (const auto locked = child.lock())
            {
                if (!locked->GetActive())
                {
                    continue;
                }

                locked->PreRender(dt);
            }
        }
    }

    void Object::Update(const float& dt)
    {
        for (const auto& component : m_components_ | std::views::values)
        {
            if (!component->GetActive())
            {
                continue;
            }

            component->Update(dt);
        }

        for (const auto& child : m_children_cache_ | std::views::values)
        {
            if (const auto locked = child.lock())
            {
                if (!locked->GetActive())
                {
                    continue;
                }

                locked->Update(dt);
            }
        }
    }
} // namespace Engine::Abstract
