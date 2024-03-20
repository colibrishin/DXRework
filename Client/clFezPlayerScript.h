#pragma once
#include "Client.h"
#include <egScript.h>

namespace Client::Scripts
{
  class FezPlayerScript : public Engine::Script
  {
  public:
    CLIENT_SCRIPT_T(FezPlayerScript, SCRIPT_T_FEZ_PLAYER)

    explicit FezPlayerScript(const WeakObjectBase& owner)
      : Script(SCRIPT_T_FEZ_PLAYER, owner),
        m_state_(CHAR_STATE_IDLE),
        m_prev_state_(CHAR_STATE_IDLE),
        m_rotation_count_(0) { }

    void Initialize() override;
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

    FezPlayerScript();

    void UpdateMove();
    void UpdateRotate();
    void UpdateJump();
    void UpdateGrounded();

    void MoveCameraToChild() const;

    eCharacterState m_state_;
    eCharacterState m_prev_state_;
    UINT m_rotation_count_;

  };
}
