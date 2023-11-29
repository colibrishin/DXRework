#pragma once
#include "egRenderable.hpp"

namespace Engine::Abstract
{
	class ActorInterface : public Renderable
	{
	public:
		virtual ~ActorInterface() override = default;

	protected:
		friend class Scene;
		friend class Object;
		friend class Component;

		virtual void OnCreate() = 0;
		virtual void OnDestroy() = 0;
		virtual void OnSceneChanging() = 0;
		virtual void OnSceneChanged() = 0;
		virtual void OnLayerChanging() = 0;
		virtual void OnLayerChanged() = 0;
	};
}
