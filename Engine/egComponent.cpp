#include "pch.hpp"
#include "egComponent.hpp"

SERIALIZER_ACCESS_IMPL(Engine::Abstract::Component,
	_ARTAG(_BSTSUPER(Renderable))
	_ARTAG(m_local_id_)
	_ARTAG(m_priority_))

namespace Engine::Abstract
{
	TypeName Component::GetVirtualTypeName() const
	{
		return typeid(Component).name();
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

	Component::Component(eComponentPriority priority, const WeakObject& owner): m_local_id_(g_invalid_id),
		m_priority_(priority),
		m_owner_(owner), m_b_ticked_(false), m_b_active_(true)
	{
	}
}
