#pragma once
#include "egEntity.hpp"

namespace Engine::Abstract
{
	class Renderable : public Entity
	{
	public:
		virtual void PreRender() = 0;
		virtual void Render() = 0;
	};
}
