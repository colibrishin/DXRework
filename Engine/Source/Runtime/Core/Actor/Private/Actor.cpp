#include "../Public/Actor.h"
#include "Actor.generated.h"

#if WITH_EDITOR
#include "UIInterface.h"
#endif

#include "Layer/Public/Layer.h"
#include "Scene/Public/Scene.h"

namespace Engine::Abstracts
{
	Actor::Actor(const Actor& other)
		: Renderable(other)
	{
		m_assigned_scene_ = other.m_assigned_scene_;
		m_layer_          = other.m_layer_;
		m_local_id_       = other.m_local_id_;
	}

	LayerSizeType Actor::GetLayer() const
	{
		return m_layer_;
	}

	Weak<Scene> Actor::GetScene() const
	{
		return m_assigned_scene_;
	}

	const LocalActorID& Actor::GetLocalID() const
	{
		return m_local_id_;
	}

#if WITH_EDITOR
	void Actor::OnUIUpdate(UIContext* const parent, const float dt)
	{
		if (parent)
		{
			Renderable::OnUIUpdate(parent, dt);

			UIInterface& ui = UIInterfaceAccessor::GetInterface();

			if (const Strong<Scene>& scene = GetScene().lock())
			{
				*(parent) |= ui.NewLabelAndText( this, "Layer", {"Layer", const_cast<EntityName&>((*scene)[GetLayer()]->GetName()), false});
			}
			else
			{
				static std::string empty;
				*(parent) |= ui.NewLabelAndText( this, "Layer", {"Layer", empty, false});
			}

			*(parent) |= ui.NewLabelAndUInt( this, "LocalID", {"Local ID", const_cast<LocalActorID&>(GetLocalID()), 0.f, 0, 0, false});
		}
	}
#endif

	Actor::Actor()
		: m_assigned_scene_({}),
		  m_layer_(static_cast<LayerSizeType>(0)),
		  m_local_id_(g_invalid_id) { }

	void Actor::SetLayer(LayerSizeType layer)
	{
		m_layer_ = layer;
		onLayerChange.Broadcast(layer);
	}

	void Actor::SetScene(const Weak<Scene>& scene)
	{
		m_assigned_scene_ = scene;
	}

	void Actor::SetLocalID(const LocalActorID id)
	{
		if (m_assigned_scene_.lock())
		{
			m_local_id_ = id;
		}
	}
} // namespace Engine::Abstract
