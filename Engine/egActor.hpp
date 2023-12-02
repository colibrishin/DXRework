#pragma once
#include "egActorInterfce.hpp"
#include "egType.hpp"
#include "egRenderable.hpp"

namespace Engine
{
	class Scene;
}

namespace Engine::Abstract
{
	class Actor : public ActorInterface
	{
	public:
		~Actor() override = default;

		eLayerType GetLayer() const { return m_layer_; }
		WeakScene GetScene() const { return m_assigned_scene_; }

		void SetLayer(eLayerType layer);
		void SetScene(EntityID scene_id);

	protected:
		explicit Actor(const WeakScene& scene, const eLayerType layer) : m_assigned_scene_(scene), m_layer_(layer)
		{
		}

	private:
		friend class boost::serialization::access;
		WeakScene m_assigned_scene_;
		eLayerType m_layer_;

	};
}
