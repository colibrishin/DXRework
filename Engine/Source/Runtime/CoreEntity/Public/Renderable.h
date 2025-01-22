#pragma once

#include "Source/Runtime/CoreEntity/Public/Entity.hpp"

namespace Engine::Abstracts
{
	class ENGINE_COREENTITY_API Renderable : public Entity
	{
	public:
		virtual void PreRender(const float dt) = 0;
		virtual void Render(const float dt) = 0;
		virtual void PostRender(const float dt) = 0;

	protected:
		Renderable() = default;

	};
} // namespace Engine::Abstract

