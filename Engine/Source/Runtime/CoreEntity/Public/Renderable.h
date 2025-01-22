#pragma once
#include "Source/Runtime/CoreEntity/Public/Entity.h"
#include "Renderable.generated.h"

namespace Engine::Abstracts
{
	ECLASS(abstract)
	class ENGINE_COREENTITY_API Renderable : public Entity
	{
	public:
		GENERATE_BODY

		virtual void PreRender(const float dt) = 0;
		virtual void Render(const float dt) = 0;
		virtual void PostRender(const float dt) = 0;

	protected:
		Renderable() = default;
	};
} // namespace Engine::Abstract
