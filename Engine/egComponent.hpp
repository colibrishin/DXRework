#pragma once
#include "egCommon.hpp"
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
		eComponentPriority GetPriority() const { return m_priority_; }

	protected:
		Component(eComponentPriority priority, const std::weak_ptr<Object>& owner) : m_owner_(owner), m_priority_(priority) {}

	private:
		std::weak_ptr<Object> m_owner_;
		eComponentPriority m_priority_;

	};
}
