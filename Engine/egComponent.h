#pragma once
#include "egCommon.hpp"
#include "egRenderable.h"

namespace Engine::Abstract
{
	class Component : public Entity
	{
	public:
		~Component() override       = default;
		Component(const Component&) = default;

		WeakObjectBase   GetOwner() const;
		eComponentType   GetComponentType() const;
		LocalComponentID GetLocalID() const;
		bool             IsTicked() const;
		bool             GetActive() const;

		virtual void SetActive(bool active);
		void         Initialize() override;
		void         PostUpdate(const float& dt) override;

		void                          OnDeserialized() override;
		void                          OnImGui() override;
		[[nodiscard]] StrongComponent Clone() const;

	protected:
		Component(eComponentType type, const WeakObjectBase& owner);

	private:
		SERIALIZE_DECL
		friend class ObjectBase;

		[[nodiscard]] virtual StrongComponent cloneImpl() const = 0;

		void SetOwner(const WeakObjectBase& owner);

		void SetLocalID(LocalComponentID id)
		{
			if (const auto locked = m_owner_.lock())
			{
				m_local_id_ = id;
			}
		}

	private:
		LocalComponentID m_local_id_;
		eComponentType   m_type_;

		// Non-serialized
		WeakObjectBase m_owner_;
		bool           m_b_ticked_;
		bool           m_b_active_;
	};
} // namespace Engine::Abstract

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Engine::Abstract::Component)
