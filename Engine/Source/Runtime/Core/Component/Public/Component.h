#pragma once
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/CoreEntity/Public/Entity.h"

#include "Component.generated.h"

// Cloning component declaration macro
#define COMP_CLONE_DECL Engine::Strong<Engine::Abstracts::Component> cloneImpl() const override;
// Cloning component implementation macro
#define COMP_CLONE_IMPL(CLASS) Engine::Strong<Engine::Abstracts::Component> CLASS::cloneImpl() const { return boost::make_shared<CLASS>(*this); }

namespace Engine
{
	struct ENGINE_CORE_API ComponentPriorityComparer
	{
		bool operator()(Weak<Abstracts::Component> Left, Weak<Abstracts::Component> Right) const;
	};

	using ComponentType = HashType;
}

namespace Engine::Abstracts
{
	class ObjectBase;

	ECLASS(abstract, serialize)
	class ENGINE_CORE_API Component : public Abstracts::Entity
	{
	public:
		GENERATE_BODY

		~Component() override       = default;
		Component(const Component&) = default;

		Weak<ObjectBase>   GetOwner() const;
		LocalComponentID GetLocalID() const;
		bool             IsTicked() const;
		bool             GetActive() const;

		virtual void                       EndPlay( const float dt );
		virtual void                       BeginPlay( const float dt );
		virtual void SetActive(bool active);
		virtual eComponentUpdatePriorities GetUpdatePriority() const = 0;
		void         Initialize() override;
		void         PostUpdate(const float dt) override;

#if WITH_EDITOR
		void         OnNameChanged() override;
#endif

		void                          OnDeserialized() override;
		[[nodiscard]] Strong<Component> Clone() const;

	protected:
		Component(const Weak<ObjectBase>& owner);

	private:
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
		EPROPERTY()
		LocalComponentID m_local_id_{};

		// Non-serialized
		Weak<ObjectBase> m_owner_{};
		bool             m_b_ticked_{};
		bool             m_b_active_{};
	};
} // namespace Engine::Abstracts

namespace Engine
{
	template struct ENGINE_CORE_API FactoryTemplate<Engine::Abstracts::Component, const Weak<Engine::Abstracts::ObjectBase>&>;
	using ComponentFactory = FactoryTemplate<Engine::Abstracts::Component, const Weak<Engine::Abstracts::ObjectBase>&>;
}