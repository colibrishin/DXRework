#include "pch.h"
#include "egScript.h"

SERIALIZE_IMPL
(
 Engine::Script,
 _ARTAG(_BSTSUPER(Renderable))
)

Engine::Script::Script(eScriptType type, const WeakObjectBase& owner)
  : m_type_(type),
    m_b_active_(true)
{
  if (const auto obj = owner.lock())
  {
    m_owner_ = obj;
  }
}

void Engine::Script::SetActive(const bool active) { m_b_active_ = active; }

Engine::StrongScript Engine::Script::Clone(const WeakObjectBase& owner) const
{
  auto clone = cloneImpl();
  clone->SetOwner(owner);
  return clone;
}

void Engine::Script::OnSerialized() {}

Engine::Script::Script()
  : m_type_(),
    m_b_active_(true) {}

void Engine::Script::SetOwner(const WeakObjectBase& owner)
{
  m_owner_ = owner;
}
