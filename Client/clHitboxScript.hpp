#pragma once
#include "clPlayerScript.h"
#include "egObject.hpp"
#include "egScript.h"

namespace Client::Scripts
{
  class HitboxScript : public Script
  {
  public:
	  CLIENT_SCRIPT_T(HitboxScript, SCRIPT_T_HITBOX)

    explicit HitboxScript(const WeakObjectBase& owner);

	  ~HitboxScript() override;

	  void Hit(const float dmg) const;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;

  private:
    SERIALIZE_DECL
    SCRIPT_CLONE_DECL

    HitboxScript();

  private:
    float m_modifier_;

  };
}

BOOST_CLASS_EXPORT_KEY(Client::Scripts::HitboxScript)