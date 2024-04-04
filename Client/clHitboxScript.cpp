#include "pch.h"
#include "clHitboxScript.hpp"

SERIALIZE_IMPL(Client::Scripts::HitboxScript, _ARTAG(_BSTSUPER(Script)) _ARTAG(m_modifier_))

namespace Client::Scripts
{
  SCRIPT_CLONE_IMPL(HitboxScript)

  HitboxScript::HitboxScript(const WeakObjectBase& owner): Script(SCRIPT_T_HITBOX, owner),
                                                              m_modifier_(1.f) {}

  HitboxScript::~HitboxScript() = default;

  void HitboxScript::Hit(const float dmg) const
  {
    const auto damage = dmg * m_modifier_;
	  
    if (const auto hitbox = GetOwner().lock())
    {
      if (const auto player = hitbox->GetParent().lock())
      {
        if (const auto cc = player->GetScript<PlayerScript>().lock())
        {
          cc->Hit(damage);
        }
      }
    }
  }

  void HitboxScript::PreUpdate(const float& dt) { }

  void HitboxScript::Update(const float& dt) { }

  void HitboxScript::PostUpdate(const float& dt) { }

  void HitboxScript::FixedUpdate(const float& dt) { }

  void HitboxScript::PreRender(const float& dt) { }

  void HitboxScript::Render(const float& dt) { }

  void HitboxScript::PostRender(const float& dt) { }

  void HitboxScript::OnCollisionEnter(const WeakCollider& other) {}

  void HitboxScript::OnCollisionContinue(const WeakCollider& other) {}

  void HitboxScript::OnCollisionExit(const WeakCollider& other) {}

  HitboxScript::HitboxScript() = default;
}
