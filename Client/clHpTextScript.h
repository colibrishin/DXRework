#pragma once
#include "clPlayerScript.h"
#include "egObject.hpp"
#include "egScript.h"

namespace Client::Scripts
{
  class HpTextScript : public Script
  {
  public:
	  CLIENT_SCRIPT_T(HpTextScript, SCRIPT_T_HP_TEXT)

    explicit HpTextScript(const WeakObjectBase& owner)
      : Script(SCRIPT_T_HP_TEXT, owner) {}

	  ~HpTextScript() override = default;

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;

  private:
    HpTextScript();
    SERIALIZE_DECL
    SCRIPT_CLONE_DECL

  };
}

BOOST_CLASS_EXPORT_KEY(Client::Scripts::HpTextScript)