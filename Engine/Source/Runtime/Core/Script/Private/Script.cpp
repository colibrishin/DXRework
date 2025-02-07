#include "../Public/Script.h"
#include "Script.generated.h"

namespace Engine
{
	std::unordered_map<Engine::ScriptType, Engine::ScriptGeneratorSignature> Engine::ScriptFactory::m_script_generators_ = {};
	
	Script::Script(const Weak<Abstracts::ObjectBase>& owner) :
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

	Script::Script() :
	m_b_active_(true) {}

	void Script::SetOwner(const Weak<Abstracts::ObjectBase>& owner)
	{
		m_owner_ = owner;
	}
}
