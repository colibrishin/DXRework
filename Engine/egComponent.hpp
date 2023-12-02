#pragma once
#include "egActorInterfce.hpp"
#include "egCommon.hpp"
#include "egEntity.hpp"
#include "egRenderable.hpp"

namespace Engine::Abstract
{
	class Object;

	class Component : public ActorInterface
	{
	public:
		~Component() override = default;
		Component(const Component&) = default;

		void Initialize() final;
		virtual void Initialize_INTERNAL() = 0;
		WeakObject GetOwner() const { return m_owner_; }
		eComponentPriority GetPriority() const { return m_priority_; }

	protected:
		Component(eComponentPriority priority, const WeakObject& owner) : m_owner_(owner), m_priority_(priority) {}

	private:
		friend class boost::serialization::access;
		WeakObject m_owner_;
		eComponentPriority m_priority_;

	};

	inline void Component::Initialize()
	{
		Initialize_INTERNAL();
		OnCreate();
	}
}
