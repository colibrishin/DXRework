#include "pch.h"
#include "egScript.h"

SERIALIZER_ACCESS_IMPL
(
 Engine::Script,
 _ARTAG(_BSTSUPER(Renderable))
)

Engine::Script::Script(eScriptType type, const WeakObject& owner)
  : m_type_(type),
    m_b_active_(false) { if (const auto obj = owner.lock()) { m_owner_ = obj; } }

void Engine::Script::SetActive(const bool active) { m_b_active_ = active; }

Engine::Script::Script()
  : m_type_(),
    m_b_active_(false) {}
