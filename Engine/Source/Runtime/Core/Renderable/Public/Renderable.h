#pragma once

#include "Source/Runtime/Core/Entity/Public/Entity.hpp"

namespace Engine::Abstracts
{
	class CORE_API Renderable : public Entity
	{
	public:
		virtual void PreRender(const float& dt) = 0;
		virtual void Render(const float& dt) = 0;
		virtual void PostRender(const float& dt) = 0;

	protected:
		Renderable() = default;

	};
} // namespace Engine::Abstract

