#pragma once
#include "egEntity.hpp"
#include "egRenderable.hpp"

namespace Engine::Abstract
{
	class Component : public Renderable
	{
	public:
		virtual ~Component() override = default;
		Component(const Component&) = default;

	protected:
		Component() = default;
	};
}
