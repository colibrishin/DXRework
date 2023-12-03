#include "pch.hpp"
#include "egComponent.hpp"

SERIALIZER_ACCESS_IMPL(Engine::Abstract::Component,
	_ARTAG(_BSTSUPER(Renderable))
	_ARTAG(m_local_id_)
	_ARTAG(m_priority_))

namespace Engine::Abstract
{
	Component::Component(eComponentPriority priority, const WeakObject& owner): m_local_id_(g_invalid_id), m_owner_(owner),
		m_priority_(priority)
	{
	}
}
