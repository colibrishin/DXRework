#pragma once
#include <boost/serialization/assume_abstract.hpp>

#include "Source/Runtime/Core/Entity/Public/Entity.hpp"

// Static Component type, this should be added to every component
#define COMPONENT_T(enum_val) static constexpr eComponentType ctype = enum_val;

// Cloning component declaration macro
#define COMP_CLONE_DECL Strong<Engine::Abstracts::Component> cloneImpl() const override;
// Cloning component implementation macro
#define COMP_CLONE_IMPL(CLASS) Strong<Engine::Abstracts::Component> CLASS::cloneImpl() const { return boost::make_shared<CLASS>(*this); }

namespace Engine
{
	struct CORE_API ComponentPriorityComparer
	{
		bool operator()(Weak<Abstracts::Component> Left, Weak<Abstracts::Component> Right) const;
	};

	enum CORE_API eComponentType : uint8_t
	{
		COM_T_UNK = 0,
		COM_T_TRANSFORM,
		COM_T_COLLIDER,
		COM_T_RIDIGBODY,
		COM_T_STATE,
		COM_T_SOUND_PLAYER,
		COM_T_ANIMATOR,
		COM_T_RENDERER,
		COM_T_SCRIPT,
	};

	template <typename T>
	struct CORE_API which_component
	{
		static constexpr eComponentType value = T::ctype;
	};
}

namespace Engine::Abstracts
{
	class ObjectBase;

	class CORE_API Component : public Abstracts::Entity
	{
	public:
		~Component() override       = default;
		Component(const Component&) = default;

		Weak<ObjectBase>   GetOwner() const;
		eComponentType   GetComponentType() const;
		LocalComponentID GetLocalID() const;
		bool             IsTicked() const;
		bool             GetActive() const;

		virtual void SetActive(bool active);
		void         Initialize() override;
		void         PostUpdate(const float& dt) override;

		void                          OnDeserialized() override;
		[[nodiscard]] Strong<Component> Clone() const;

	protected:
		Component(eComponentType type, const Weak<ObjectBase>& owner);

	private:
		SERIALIZE_DECL
		friend class ObjectBase;

		[[nodiscard]] virtual Strong<Component> cloneImpl() const = 0;

		void SetOwner(const Weak<ObjectBase>& owner);

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
		Weak<ObjectBase> m_owner_{};
		bool             m_b_ticked_;
		bool             m_b_active_;
	};
} // namespace Engine::Abstract

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Engine::Abstract::Component)
