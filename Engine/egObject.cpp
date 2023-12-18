#include "pch.h"

#include "egObject.hpp"
#include "egCollider.hpp"
#include "egCollision.h"
#include "egManagerHelper.hpp"
#include "egMesh.h"
#include "egRigidbody.h"
#include "egTransform.h"
#include "egComponent.h"

SERIALIZER_ACCESS_IMPL(
                       Engine::Abstract::Object,
                       _ARTAG(_BSTSUPER(Actor)) _ARTAG(m_components_))

namespace Engine::Abstract
{
    template void Object::DispatchComponentEvent(
        Engine::Components::Collider& lhs,
        Engine::Components::Collider& rhs);

    template <typename T>
    void Object::DispatchComponentEvent(T& lhs, T& rhs)
    {
        if constexpr (std::is_base_of_v<Component, T>)
        {
            if constexpr (std::is_same_v<Engine::Components::Collider, T>)
            {
                const auto speculation_check = GetCollisionDetector().IsSpeculated(
                 lhs.GetOwner().lock()->GetID(), rhs.GetOwner().lock()->GetID());

                if (speculation_check)
                {
                    lhs.AddSpeculationObject(rhs.GetOwner().lock()->GetID());
                }

                const auto collision_check = GetCollisionDetector().IsCollided(
                                                                               lhs.GetOwner().lock()->GetID(),
                                                                               rhs.GetOwner().lock()->GetID());

                const auto collision_frame = GetCollisionDetector().IsCollidedInFrame(
                 lhs.GetOwner().lock()->GetID(), rhs.GetOwner().lock()->GetID());

                if (collision_frame)
                {
                    lhs.GetOwner().lock()->OnCollisionEnter(rhs);
                    lhs.AddCollidedObject(rhs.GetOwner().lock()->GetID());
                }
                else if (collision_check)
                {
                    lhs.GetOwner().lock()->OnCollisionContinue(rhs);
                }
                else if (!collision_check && !collision_frame)
                {
                    lhs.GetOwner().lock()->OnCollisionExit(rhs);
                    lhs.RemoveCollidedObject(rhs.GetOwner().lock()->GetID());
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

    void Object::OnCollisionEnter(const Engine::Components::Collider& other)
    {
        if (!GetComponent<Engine::Components::Collider>().lock())
        {
            throw std::exception("Object has no collider");
        }
    }

    void Object::OnCollisionContinue(const Engine::Components::Collider& other)
    {
        if (!GetComponent<Engine::Components::Collider>().lock())
        {
            throw std::exception("Object has no collider");
        }
    }

    void Object::OnCollisionExit(const Engine::Components::Collider& other)
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

        for (const auto& component : m_priority_sorted_)
        {
            if (const auto locked = component.lock())
            {
                if (!locked->GetActive())
                {
                    continue;
                }

                locked->Render(dt);
            }
            else
            {
                m_priority_sorted_.erase(component);
            }
        }
    }

    void Object::PostRender(const float& dt)
    {
        for (const auto& component : m_priority_sorted_)
        {
            if (const auto locked = component.lock())
            {
                if (!locked->GetActive())
                {
                    continue;
                }

                locked->PostRender(dt);
            }
            else
            {
                m_priority_sorted_.erase(component);
            }
        }
    }

    void Object::FixedUpdate(const float& dt)
    {
        for (const auto& component : m_priority_sorted_)
        {
            if (const auto locked = component.lock())
            {
                if (!locked->GetActive())
                {
                    continue;
                }

                locked->FixedUpdate(dt);
            }
            else
            {
                m_priority_sorted_.erase(component);
            }
        }
    }

    void Object::OnDeserialized()
    {
        Actor::OnDeserialized();

        for (const auto& val : m_components_ | std::views::values)
        {
            for (const auto& comps : val)
            {
                comps->OnDeserialized();
                comps->SetOwner(GetSharedPtr<Object>());
                m_assigned_component_ids_.insert(comps->GetLocalID());
                m_priority_sorted_.insert(comps);
            }
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
                for (const auto& pointer : m_components_ | std::views::values)
                {
                    for (const auto& comp : pointer)
                    {
                        if (ImGui::TreeNode(comp->GetTypeName().c_str()))
                        {
                            comp->OnImGui();
                            ImGui::TreePop();
                            ImGui::Spacing();
                        }
                    }
                }

                ImGui::TreePop();
                ImGui::Spacing();
            }

            ImGui::Unindent(2);
            ImGui::End();
        }
    }

    const std::set<WeakComponent, ComponentPriorityComparer>& Object::GetAllComponents() const
    {
        return m_priority_sorted_;
    }

    void Object::PreUpdate(const float& dt)
    {
        for (const auto& component : m_priority_sorted_)
        {
            if (const auto locked = component.lock())
            {
                if (!locked->GetActive())
                {
                    continue;
                }

                locked->PreUpdate(dt);
            }
            else
            {
                m_priority_sorted_.erase(component);
            }
        }
    }

    void Object::PreRender(const float& dt)
    {
        for (const auto& component : m_priority_sorted_)
        {
            if (const auto locked = component.lock())
            {
                if (!locked->GetActive())
                {
                    continue;
                }

                locked->PreRender(dt);
            }
            else
            {
                m_priority_sorted_.erase(component);
            }
        }
    }

    void Object::Update(const float& dt)
    {
        for (const auto& component : m_priority_sorted_)
        {
            if (const auto locked = component.lock())
            {
                if (!locked->GetActive())
                {
                    continue;
                }

                locked->Update(dt);
            }
            else
            {
                m_priority_sorted_.erase(component);
            }
        }
    }
} // namespace Engine::Abstract
