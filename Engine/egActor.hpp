#pragma once
#include "egType.hpp"
#include "egRenderable.hpp"

namespace Engine::Abstract
{
	class Actor : public Renderable
	{
	public:
		~Actor() override = default;

		eLayerType GetLayer() const { return m_layer_; }
		WeakScene GetScene() const { return m_assigned_scene_; }

	protected:
		explicit Actor() : m_assigned_scene_({}), m_layer_(LAYER_NONE)
		{
		}

	private:
		friend class boost::serialization::access;
		friend class Scene;

		void SetLayer(eLayerType layer) { m_layer_ = layer; }
		void SetScene(const WeakScene& scene) { m_assigned_scene_ = scene; }

		WeakScene m_assigned_scene_;
		eLayerType m_layer_;

	};
}
