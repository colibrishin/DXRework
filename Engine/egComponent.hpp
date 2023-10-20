#pragma once
#include "egEntity.hpp"
#include "egRenderable.hpp"

namespace Engine::Abstract
{
	class Object;

	class Component : public Renderable
	{
	public:
		~Component() override = default;
		Component(const Component&) = default;

		std::weak_ptr<Object> GetOwner() const { return m_owner_; }

	protected:
		Component(const std::weak_ptr<Object>& owner) : m_owner_(owner) {}

	private:
		std::weak_ptr<Object> m_owner_;
	};
}
