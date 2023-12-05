#pragma once
#include "egCommon.hpp"
#include "egEntity.hpp"

namespace Engine::Abstract
{
	class Renderable : public Entity
	{
	public:
		virtual void PreRender(const float dt) = 0;
		virtual void Render(const float dt) = 0;

	private:
		SERIALIZER_ACCESS
	};
}
