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
		ComponentID GetLocalID() const { return m_local_id_; }

	protected:
		Component(eComponentPriority priority, const WeakObject& owner);

	private:
		SERIALIZER_ACCESS
		friend class Object;

		void SetOwner(const WeakObject& owner)
		{
			m_owner_ = owner;
		}
		void SetLocalID(ComponentID id)
		{
			if (const auto locked = m_owner_.lock())
			{
				m_local_id_ = id;
			}
		}

		ComponentID m_local_id_;
		eComponentPriority m_priority_;

		// Non-serialized
		WeakObject m_owner_;

	};
}
