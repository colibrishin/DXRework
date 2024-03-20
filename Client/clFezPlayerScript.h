#pragma once
#include "Client.h"
#include <egScript.h>

namespace Client::Scripts
{
  class FezPlayerScript : public Engine::Script
  {
  public:
    // Vector3::Up is non-const static
    constexpr static Vector3 s_up = {0, 1, 0};
    constexpr static float s_rotation_speed = 1.f;

    inline static const Quaternion s_rotations[4] = 
    {
      Quaternion::CreateFromAxisAngle(s_up, 0.0f),
      Quaternion::CreateFromAxisAngle(s_up, XMConvertToRadians(90.0f)),
      Quaternion::CreateFromAxisAngle(s_up, XMConvertToRadians(180.0f)),
      Quaternion::CreateFromAxisAngle(s_up, XMConvertToRadians(270.0f))
    };

    CLIENT_SCRIPT_T(FezPlayerScript, SCRIPT_T_FEZ_PLAYER)

    explicit FezPlayerScript(const WeakObjectBase& owner)
      : Script(SCRIPT_T_FEZ_PLAYER, owner),
        m_state_(CHAR_STATE_IDLE),
        m_prev_state_(CHAR_STATE_IDLE),
        m_rotation_count_(0),
        m_rotate_allowed_(true) { }

    void Initialize() override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void SetRotateAllowed(const bool allowed) { m_rotate_allowed_ = allowed; }

  protected:
    void OnCollisionEnter(const WeakCollider& other) override;
    void OnCollisionContinue(const WeakCollider& other) override;
    void OnCollisionExit(const WeakCollider& other) override;

  private:
    SERIALIZE_DECL
    SCRIPT_CLONE_DECL

    FezPlayerScript();

    void UpdateMove();
    void UpdateRotate(const float dt);
    void UpdateJump();
    void UpdateGrounded();

    void MoveCameraToChild() const;

    eCharacterState m_state_;
    eCharacterState m_prev_state_;

    UINT m_rotation_count_;

    float m_accumulated_dt_;

    // Flag for whether player has red hat.
    bool m_rotate_allowed_;

  };
} // namespace Client::Scripts

BOOST_CLASS_EXPORT_KEY(Client::Scripts::FezPlayerScript)