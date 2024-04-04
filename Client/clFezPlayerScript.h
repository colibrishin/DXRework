#pragma once
#include "Client.h"
#include <egScript.h>

#include "clCubifyScript.h"

namespace Client::Scripts
{
  class FezPlayerScript : public Engine::Script
  {
  public:
    // Vector3::Up is non-const static
    constexpr static Vector3 s_up = {0, 1, 0};
    constexpr static float s_rotation_speed = 1.f;

    // Clockwise movement
    inline static const Quaternion s_rotations[4] = 
    {
      Quaternion::CreateFromAxisAngle(s_up, 0.0f),
      Quaternion::CreateFromAxisAngle(s_up, -XMConvertToRadians(90.f)),
      Quaternion::CreateFromAxisAngle(s_up, -XMConvertToRadians(180.f)),
      Quaternion::CreateFromAxisAngle(s_up, -XMConvertToRadians(270.f))
    };

    CLIENT_SCRIPT_T(FezPlayerScript, SCRIPT_T_FEZ_PLAYER)

    explicit FezPlayerScript(const WeakObjectBase& owner)
      : Script(SCRIPT_T_FEZ_PLAYER, owner),
        m_state_(CHAR_STATE_IDLE),
        m_prev_state_(CHAR_STATE_IDLE),
        m_rotation_count_(0),
        m_accumulated_dt_(0),
        m_rotate_allowed_(true),
        m_rotate_finished_(false),
        m_rotate_consecutive_(false),
        m_b_climbing_(false),
        m_b_vaulting_(false) { }

    void Initialize() override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void SetRotateAllowed(const bool allowed) { m_rotate_allowed_ = allowed; }

    eCharacterState GetState() const { return m_state_; }
    eCharacterState GetPrevState() const { return m_prev_state_; }
    UINT GetRotationOffset() const { return m_rotation_count_; }

    void OnImGui() override;

  protected:
    void OnCollisionEnter(const WeakCollider& other) override;
    void OnCollisionContinue(const WeakCollider& other) override;
    void OnCollisionExit(const WeakCollider& other) override;

  private:
    SERIALIZE_DECL
    SCRIPT_CLONE_DECL

    FezPlayerScript();

    constexpr static float s_jump_initial_speed = g_gravity_acc * 3.f;
    constexpr static float s_jump_speed = g_gravity_acc;
    constexpr static float s_jump_apex = 10.f;

    // Utilities
    void IgnoreCollision() const;
    void ApplyCollision() const;
    void IgnoreGravity() const;
    void ApplyGravity() const;
    void IgnoreLerp() const;
    void ApplyLerp() const;
    void Fullstop() const;
    void MoveCameraToChild() const;

    // State changes
    void UpdateMove();
    void UpdateRotate(const float dt);
    void UpdateGrounded();

    void UpdateInitialJump();
    void UpdateJump();
    void UpdateFall();

    void UpdateInitialClimb();
    void UpdateClimb();

    void UpdateInitialVault();
    void UpdateVault();

    // Subroutine for state changes
    void DoInitialJump(const StrongRigidbody& rb, const Vector3& up);

    bool doInitialClimb(
      const StrongTransform& tr, const Vector3& pos, const WeakObjectBase& obj, bool& continues);

    bool movePlayerToNearestCube(
      const StrongTransform & tr, 
      const boost::shared_ptr<CubifyScript>& script,
      const Vector3& player_pos) const;

    void doDownVault(const StrongTransform& tr);

    eCharacterState m_state_;
    eCharacterState m_prev_state_;

    // Rotation variables
    // Count of 90 degree rotations
    UINT m_rotation_count_;
    // Animation time for rotation
    float m_accumulated_dt_;

    // Last position of player when rotating
    Vector3 m_last_spin_position_;

    // Flag for whether player has red hat.
    bool m_rotate_allowed_;
    // Flag for whether player has finished rotating
    bool m_rotate_finished_;
    // Flag for whether player has consecutive rotations
    bool m_rotate_consecutive_;

    // Climb variables
    bool m_b_climbing_;
    bool m_b_vaulting_;

  };
} // namespace Client::Scripts

BOOST_CLASS_EXPORT_KEY(Client::Scripts::FezPlayerScript)