#include "pch.hpp"
#include "egActor.hpp"

SERIALIZER_ACCESS_IMPL(Engine::Abstract::Actor,
	_ARTAG(m_layer_)
	_ARTAG(m_local_id_))

namespace Engine::Abstract
{
	eLayerType Actor::GetLayer() const
	{
		return m_layer_;
	}

	WeakScene Actor::GetScene() const
	{
		return m_assigned_scene_;
	}

	ActorID Actor::GetLocalID() const
	{
		return m_local_id_;
	}

	void Actor::SetLayer(eLayerType layer)
	{
		m_layer_ = layer;
	}

	void Actor::SetScene(const WeakScene& scene)
	{
		m_assigned_scene_ = scene;
	}

	void Actor::SetLocalID(const ActorID id)
	{
		if (m_assigned_scene_.lock())
		{
			m_local_id_ = id;
		}
	}
}
