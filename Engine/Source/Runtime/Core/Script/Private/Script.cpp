#include "../Public/Script.h"

namespace Engine
{
	Script::Script(const ScriptSizeType type, const Weak<Abstracts::ObjectBase>& owner)
	: m_type_(type),
	  m_b_active_(true)
	{
		if (const auto obj = owner.lock())
		{
			m_owner_ = obj;
		}
	}

	void Script::SetActive(const bool active)
	{
		m_b_active_ = active;
	}

	Strong<Script> Script::Clone(const Weak<Abstracts::ObjectBase>& owner) const
	{
		auto clone = cloneImpl();
		clone->SetOwner(owner);
		return clone;
	}

	Script::Script()
		: m_type_(),
		  m_b_active_(true) {}

	void Script::SetOwner(const Weak<Abstracts::ObjectBase>& owner)
	{
		m_owner_ = owner;
	}
}
