#pragma once
#include "egCommon.hpp"
#include "egEntity.hpp"

namespace Engine::Abstract
{
	class Renderable : public Entity
	{
	public:
		virtual void PreRender(const float& dt) = 0;
		virtual void Render(const float& dt) = 0;
		virtual void PostRender(const float& dt) = 0;

	protected:
		Renderable() = default;

	private:
		SERIALIZE_DECL
	};
} // namespace Engine::Abstract

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Engine::Abstract::Renderable)
