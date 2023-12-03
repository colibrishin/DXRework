#pragma once
#include "egCommon.hpp"
#include "egEntity.hpp"
#include "egRenderable.hpp"

namespace Engine::Abstract
{
	class Component : public Renderable
	{
	public:
		~Component() override = default;
		Component(const Component&) = default;

		WeakObject GetOwner() const { return m_owner_; }
		eComponentPriority GetPriority() const { return m_priority_; }

	protected:
		Component(eComponentPriority priority, const WeakObject& owner) : m_owner_(owner), m_priority_(priority) {}

	private:
		friend class boost::serialization::access;
		void SetOwner(const WeakObject& owner) { m_owner_ = owner; }

		WeakObject m_owner_;
		eComponentPriority m_priority_;

	};
}
