#pragma once
#include "egRenderable.hpp"

namespace Engine::Abstract
{
	class ActorInterface : public Renderable
	{
	public:
		virtual ~ActorInterface() override = default;
	protected:
		virtual void OnLayerChanging() = 0;
		virtual void OnLayerChanged() = 0;
	};
}
