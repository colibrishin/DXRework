#include "pch.h"
#include "egComponent.h"

SERIALIZER_ACCESS_IMPL(
                       Engine::Abstract::Component,
                       _ARTAG(_BSTSUPER(Entity)) _ARTAG(m_local_id_)
                       _ARTAG(m_type_))

namespace Engine::Abstract
{
    WeakObject Component::GetOwner() const
    {
        return m_owner_;
    }

    eComponentType Component::GetComponentType() const
    {
        return m_type_;
    }

    ComponentID Component::GetLocalID() const
    {
        return m_local_id_;
    }

    bool Component::IsTicked() const
    {
        return m_b_ticked_;
    }

    void Component::SetActive(bool active)
    {
        m_b_active_ = active;
    }

    void Component::PostUpdate(const float& dt)
    {
        m_b_ticked_ = true;
    }

    bool Component::GetActive() const
    {
        return m_b_active_;
    }

    void Component::OnImGui()
    {
        Entity::OnImGui();
        ImGui::Indent(2);
        ImGui::Text("Component Local ID: %lld", m_local_id_);
        ImGui::Checkbox("Component Active", &m_b_active_);
        ImGui::Unindent(2);
    }

    Component::Component(eComponentType type, const WeakObject& owner)
    : m_local_id_(g_invalid_id),
      m_type_(type),
      m_owner_(owner),
      m_b_ticked_(false),
      m_b_active_(true) {}
} // namespace Engine::Abstract
