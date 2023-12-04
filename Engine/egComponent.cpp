#include "pch.hpp"
#include "egComponent.hpp"

SERIALIZER_ACCESS_IMPL(Engine::Abstract::Component,
	_ARTAG(_BSTSUPER(Renderable))
	_ARTAG(m_local_id_)
	_ARTAG(m_priority_))

namespace Engine::Abstract
{
	void Component::OnImGui()
	{
		Renderable::OnImGui();
		ImGui::Indent(2);
		ImGui::Text("Component Local ID: %d", m_local_id_);
		ImGui::Unindent(2);
	}

	Component::Component(eComponentPriority priority, const WeakObject& owner): m_local_id_(g_invalid_id), m_priority_(priority),
	                                                                            m_owner_(owner)
	{
	}
}
