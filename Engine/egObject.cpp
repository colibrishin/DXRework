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
                       _ARTAG(_BSTSUPER(Actor)) _ARTAG(m_components_)
                       _ARTAG(m_resources_))

namespace Engine::Abstract
{
    template void Object::DispatchComponentEvent(
        Engine::Component::Collider& lhs,
        Engine::Component::Collider& rhs);

    template <typename T>
    void Object::DispatchComponentEvent(T& lhs, T& rhs)
    {
        if constexpr (std::is_base_of_v<Component, T>)
        {
            if constexpr (std::is_same_v<Engine::Component::Collider, T>)
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

    void Object::OnCollisionEnter(const Engine::Component::Collider& other)
    {
        if (!GetComponent<Engine::Component::Collider>().lock())
        {
            throw std::exception("Object has no collider");
        }
    }

    void Object::OnCollisionContinue(const Engine::Component::Collider& other)
    {
        if (!GetComponent<Engine::Component::Collider>().lock())
        {
            throw std::exception("Object has no collider");
        }
    }

    void Object::OnCollisionExit(const Engine::Component::Collider& other)
    {
        if (!GetComponent<Engine::Component::Collider>().lock())
        {
            throw std::exception("Object has no collider");
        }
    }

    void Object::Render(const float& dt)
    {
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

        if (m_culled_ && !GetProjectionFrustum().CheckRender(GetWeakPtr<Object>()))
        {
            return;
        }

        for (const auto& resource : m_resources_)
        {
            resource->Render(dt);
        }

        // Assuming other object will initialize vertex shader or so.
        GetRenderPipeline().ResetShaders();
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

        for (const auto& resource : m_resources_)
        {
            resource->PostRender(dt);
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

        for (const auto& resource : m_resources_)
        {
            resource->FixedUpdate(dt);
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

        std::set<StrongResource, ResourcePriorityComparer> resources;

        for (auto it = m_resources_.begin(); it != m_resources_.end(); ++it)
        {
            const auto resource = *it;

            const auto res = GetResourceManager().GetResource(
                                                              resource->GetTypeName(),
                                                              resource->GetName());

            resource->OnDeserialized();

            if (const auto& locked = res.lock())
            {
                resources.insert(locked);
            }
            else
            {
                GetResourceManager().AddResource(resource);
                resource->Load();
                resources.insert(resource);
            }
        }

        m_resources_ = resources;
    }

    void Object::OnImGui()
    {
        const auto id =
                GetTypeName() + " " + GetName() + " " + std::to_string(GetID());

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

            if (ImGui::TreeNode("Resources"))
            {
                for (const auto& resource : m_resources_)
                {
                    resource->OnImGui();
                }

                ImGui::TreePop();
                ImGui::Spacing();
            }

            ImGui::Unindent(2);
            ImGui::End();
        }
    }

    TypeName Object::GetVirtualTypeName() const
    {
        return typeid(Object).name();
    }

    void Object::AddResource(const StrongResource& resource) {
        m_resources_.insert(resource);
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

        for (const auto& resource : m_resources_)
        {
            resource->PreUpdate(dt);
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

        for (const auto& resource : m_resources_)
        {
            resource->PreUpdate(dt);
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

        for (const auto& resource : m_resources_)
        {
            resource->PreUpdate(dt);
        }
    }
} // namespace Engine::Abstract
