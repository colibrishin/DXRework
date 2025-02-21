#include "../Public/Component.h"
#include "Component.generated.h"

namespace Engine
{
	bool ComponentPriorityComparer::operator()(Weak<Abstracts::Component> Left, Weak<Abstracts::Component> Right) const
	{
		if (Left.lock()->GetUpdatePriority() != Right.lock()->GetUpdatePriority())
		{
			return Left.lock()->GetUpdatePriority() < Right.lock()->GetUpdatePriority();
		}

		return Left.lock()->GetID() < Right.lock()->GetID();
	}
}

namespace Engine::Abstracts
{
	Weak<ObjectBase> Component::GetOwner() const
	{
		return m_owner_;
	}

	LocalComponentID Component::GetLocalID() const
	{
		return m_local_id_;
	}

	bool Component::IsTicked() const
	{
		return m_b_ticked_;
	}

	void Component::EndPlay( const float dt )
    {}

    void Component::BeginPlay( const float dt )
    {}

    void Component::SetActive( bool active )
	{
		m_b_active_ = active;
	}

	void Component::Initialize()
	{
		Entity::Initialize();

#if WITH_EDITOR
		SetName(GetPrettyTypeName().data());
		m_ui_info_.label = GetName();
		m_ui_info_.dialogOpened = false;
#endif
	}

	void Component::PostUpdate(const float dt)
	{
		m_b_ticked_ = true;
	}

#if WITH_EDITOR
	void Component::OnNameChanged()
	{
		Entity::OnNameChanged();
		m_ui_info_.label = GetName();
	}
#endif

	void Component::OnDeserialized()
	{
		Entity::OnDeserialized();
	}

	bool Component::GetActive() const
	{
		return m_b_active_;
	}

	Strong<Component> Component::Clone() const
	{
		const auto& cloned = cloneImpl();
		return cloned;
	}

	Component::Component(const Weak<ObjectBase>& owner) :
	m_local_id_(g_invalid_id),
	m_owner_(owner),
	m_b_ticked_(false),
	m_b_active_(true) {}

	void Component::SetOwner(const Weak<ObjectBase>& owner)
	{
		if (const auto obj = owner.lock())
		{
			m_owner_ = owner;
		}
	}
} // namespace Engine::Abstract