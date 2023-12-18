#include "pch.h"
#include "egComponent.h"

SERIALIZER_ACCESS_IMPL(
                       Engine::Abstract::Component,
                       _ARTAG(_BSTSUPER(Renderable)) _ARTAG(m_local_id_)
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

    bool Component::GetActive() const
    {
        return m_b_active_;
    }

    void Component::OnImGui()
    {
        Renderable::OnImGui();
        ImGui::Indent(2);
        ImGui::Text("Component Local ID: %lld", m_local_id_);
        ImGui::Checkbox("Component Active", &m_b_active_);
        ImGui::Unindent(2);
    }

    void Component::Render(const float& dt)
    {
        m_b_ticked_ = true;
    }

    Component::Component(eComponentType type, const WeakObject& owner)
    : m_local_id_(g_invalid_id),
      m_type_(type),
      m_owner_(owner),
      m_b_ticked_(false),
      m_b_active_(true) {}
} // namespace Engine::Abstract
