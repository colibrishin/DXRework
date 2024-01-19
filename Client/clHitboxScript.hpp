#pragma once
#include "clCharacterController.hpp"
#include "egObject.hpp"
#include "egScript.h"

namespace Client::Scripts
{
  class HitboxScript : public Script
  {
  public:
	  CLIENT_SCRIPT_T(SCRIPT_T_HITBOX)

    explicit HitboxScript(const WeakObject& owner)
      : Script(owner),
        m_modifier_(1.f) {}

	~HitboxScript() override = default;

	void Hit(const float dmg) const
  {
	  const auto damage = dmg * m_modifier_;
	  
	  if (const auto hitbox = GetOwner().lock())
	  {
	    if (const auto player = hitbox->GetParent().lock())
	    {
	      if (const auto cc = player->GetComponent<State::CharacterController>().lock())
	      {
	        cc->Hit(damage);
	      }
	    }
	  }
	}

    void PreUpdate(const float& dt) override { }

    void Update(const float& dt) override { }

    void PostUpdate(const float& dt) override { }

    void FixedUpdate(const float& dt) override { }

    void PreRender(const float& dt) override { }

    void Render(const float& dt) override { }

    void PostRender(const float& dt) override { }

  private:
	float m_modifier_;

  };
}