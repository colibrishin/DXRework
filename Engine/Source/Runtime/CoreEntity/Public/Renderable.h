#pragma once

#include "Source/Runtime/CoreEntity/Public/Entity.hpp"

POLYMORPHIC_TYPE_MAP(Engine::Abstracts::Renderable, Engine::Abstracts::Entity)

namespace Engine::Abstracts
{
	class ENGINE_COREENTITY_API Renderable : public Entity
	{
	public:
		INLINE_COMPILE_TIME_TYPENAME(Renderable)

		virtual void PreRender(const float dt) = 0;
		virtual void Render(const float dt) = 0;
		virtual void PostRender(const float dt) = 0;

	protected:
		Renderable() = default;

	};
} // namespace Engine::Abstract

