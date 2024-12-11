#include "../Public/Component.h"

namespace Engine
{
	bool ComponentPriorityComparer::operator()(Weak<Abstracts::Component> Left, Weak<Abstracts::Component> Right) const
	{
		if (Left.lock()->GetComponentType() != Right.lock()->GetComponentType())
		{
			return
				Left.lock()->GetComponentType() < Right.lock()->GetComponentType();
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

	eComponentType Component::GetComponentType() const
	{
		return m_type_;
	}

	LocalComponentID Component::GetLocalID() const
	{
		return m_local_id_;
	}

	bool Component::IsTicked() const
	{
		return m_b_ticked_;
	}

	void Component::SetActive(bool active)
	{
		m_b_active_ = active;
	}

	void Component::Initialize()
	{
		Entity::Initialize();
		SetName(GetPrettyTypeName());
	}

	void Component::PostUpdate(const float& dt)
	{
		m_b_ticked_ = true;
	}

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

	Component::Component(const eComponentType type, const Weak<ObjectBase>& owner)
		: m_local_id_(g_invalid_id),
		  m_type_(type),
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