#include "pch.h"
#include "clFezPlayerScript.h"

#include "egBaseCollider.hpp"
#include "egCamera.h"
#include "egCollisionDetector.h"
#include "egObjectBase.hpp"
#include "egRigidbody.h"
#include "egTransform.h"

SERIALIZE_IMPL
(
 Client::Scripts::FezPlayerScript,
 _ARTAG(_BSTSUPER(Engine::Script))
 _ARTAG(m_state_)
 _ARTAG(m_prev_state_)
 _ARTAG(m_rotation_count_)
 _ARTAG(m_rotate_allowed_)
)

namespace Client::Scripts
{
  void FezPlayerScript::MoveCameraToChild() const
  {
    if (const auto& owner = GetOwner().lock())
    {
      if (const auto& rb = owner->GetComponent<Components::Rigidbody>().lock())
      {
        rb->SetNoAngular(true);
      }

      if (const auto& scene = owner->GetScene().lock())
      {
        if (const auto& cam = scene->GetMainCamera().lock())
        {
          owner->AddChild(cam->GetSharedPtr<Abstract::ObjectBase>());
          cam->SetName("Camera");
          cam->SetFOV(20.f);
          cam->SetOrthogonal(true);

          if (const auto& tr = cam->GetComponent<Components::Transform>().lock()) 
          {
            tr->SetLocalPosition({ 0.0f, 0.0f, -10.0f });
          }
        }
      }
    }
  }

  void FezPlayerScript::Initialize()
  {
    Script::Initialize();
    MoveCameraToChild();
  }

  void FezPlayerScript::PreUpdate(const float& dt) {}

  void FezPlayerScript::Update(const float& dt)
  {
    switch (m_state_)
    {
    case CHAR_STATE_IDLE: 
      UpdateMove();
      UpdateRotate(dt);
      UpdateJump();
      break;
    case CHAR_STATE_WALK:
      UpdateMove();
      UpdateJump();
      break;
    case CHAR_STATE_RUN: break;
    case CHAR_STATE_JUMP: 
      UpdateMove();
      break;
    case CHAR_STATE_CLIMB: break;
    case CHAR_STATE_SWIM: break;
    case CHAR_STATE_ROTATE: 
      UpdateRotate(dt);
      break;
    case CHAR_STATE_FALL: break;
    case CHAR_STATE_ATTACK: break;
    case CHAR_STATE_HIT: break;
    case CHAR_STATE_DIE: break;
    case CHAR_STATE_MAX: break;
    default: ;
    }

    UpdateGrounded();

    m_prev_state_ = m_state_;
  }

  void FezPlayerScript::PostUpdate(const float& dt) {}

  void FezPlayerScript::FixedUpdate(const float& dt) {}

  void FezPlayerScript::PreRender(const float& dt) {}

  void FezPlayerScript::Render(const float& dt) {}

  void FezPlayerScript::PostRender(const float& dt) {}

  void FezPlayerScript::OnCollisionEnter(const WeakCollider& other) {}

  void FezPlayerScript::OnCollisionContinue(const WeakCollider& other) {}

  void FezPlayerScript::OnCollisionExit(const WeakCollider& other) {}

  SCRIPT_CLONE_IMPL(FezPlayerScript)

  FezPlayerScript::FezPlayerScript()
    : Script(SCRIPT_T_FEZ_PLAYER, {}),
      m_state_(CHAR_STATE_IDLE),
      m_prev_state_(CHAR_STATE_IDLE),
      m_rotation_count_(0),
      m_rotate_allowed_(true) { }

  void FezPlayerScript::UpdateMove()
  {
    if (GetOwner().expired()) { return; }

    const auto& owner = GetOwner().lock();
    const auto& tr = owner->GetComponent<Components::Transform>().lock();
    const auto& rb = owner->GetComponent<Components::Rigidbody>().lock();
    const auto& cam = owner->GetChild("Camera").lock();

    if (!tr || !rb || !cam) { return; }
    if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive() || !cam->GetActive()) { return; }

    const auto& right = tr->Right();
    constexpr float speed = 1.f;
    bool moving = false;

    if (GetApplication().IsKeyPressed(Keyboard::D))
    {
      rb->AddT1Force(right * speed);
      moving = true;
    }
    if (GetApplication().IsKeyPressed(Keyboard::A))
    {
      rb->AddT1Force(-right * speed);
      moving = true;
    }

    if (moving)
    {
      m_state_ = CHAR_STATE_WALK;
    }
    else
    {
      m_state_ = CHAR_STATE_IDLE;
    }
  }

  void FezPlayerScript::UpdateRotate(const float dt)
  {
    if (GetOwner().expired()) { return; }

    const auto& owner = GetOwner().lock();
    const auto& tr    = owner->GetComponent<Components::Transform>().lock();
    const auto& rb    = owner->GetComponent<Components::Rigidbody>().lock();

    if (!tr || !rb) { return; }
    if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive()) { return; }

    const auto  rot       = tr->GetLocalRotation();
    bool        rotating  = false;

    // If the player is not allowed to rotate.
    if (!m_rotate_allowed_) { return; }

    // Case where the player is rotating.
    if (m_state_ == CHAR_STATE_ROTATE)
    {
      // Wait for the rotation to be close to the target rotation.
      if (s_rotation_speed > m_accumulated_dt_)
      {
        tr->SetLocalRotation(Quaternion::Slerp(rot, s_rotations[m_rotation_count_], m_accumulated_dt_));
        m_accumulated_dt_ += dt;
        return;
      }
      // If the player is rotating and rotation is completed,
      else
      {
        // Set the rotation to the target rotation and reset the accumulated time.
        tr->SetLocalRotation(s_rotations[m_rotation_count_]);
        m_accumulated_dt_ = 0.f;

        // Set the player's state back to idle.
        m_state_ = CHAR_STATE_IDLE;
        // Clear accumulated forces (e.g., collision reaction force) and set fixed to false
        rb->FullReset();
        rb->SetFixed(false);
        return;
      }
    }

    if (GetApplication().HasKeyChanged(Keyboard::Q))
    {
      m_rotation_count_ = (m_rotation_count_ + 1) % 4;
      rotating = true;
    }
    if (GetApplication().HasKeyChanged(Keyboard::E))
    {
      m_rotation_count_ = (m_rotation_count_ + 3) % 4;
      rotating = true;
    }

    // If the player starts rotating, then set the player's state to rotate.
    // Make the player full stop.
    if (rotating)
    {
      m_state_ = CHAR_STATE_ROTATE;
      rb->FullReset();
      rb->SetFixed(true);
    }
  }

  void FezPlayerScript::UpdateJump()
  {
    if (GetOwner().expired()) { return; }

    const auto& owner = GetOwner().lock();
    const auto& tr    = owner->GetComponent<Components::Transform>().lock();
    const auto& rb    = owner->GetComponent<Components::Rigidbody>().lock();

    if (!tr || !rb) { return; }
    if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive()) { return; }

    const auto& key_state = GetApplication().GetCurrentKeyState();
    const auto&     up         = tr->Up();
    constexpr float jump_force = 400.f;
    constexpr float jump_apex = 5.f;

    // Use the continuous jump check to threshold the jump height.
    if (key_state.W || key_state.Space)
    {
      // If the player has reached the apex of the jump, then set the player's state to fall.
      if (rb->GetT0LinearVelocity().y > jump_apex)
      {
        m_state_ = CHAR_STATE_FALL;
      }
      else
      {
        rb->AddT1Force(up * jump_force);
        m_state_ = CHAR_STATE_JUMP; 
      }
    }
    else
    {
      // If the player lets go of the jump key, then set the player's state to fall.
      if (rb->GetT0LinearVelocity().y > 0.f)
      {
        m_state_ = CHAR_STATE_FALL;
      }
    }
  }

  void FezPlayerScript::UpdateGrounded()
  {
    if (GetOwner().expired()) { return; }

    const auto& owner = GetOwner().lock();
    const auto& scene = owner->GetScene().lock();
    const auto& tr    = owner->GetComponent<Components::Transform>().lock();
    const auto& rb    = owner->GetComponent<Components::Rigidbody>().lock();
    const auto& cldr = owner->GetComponent<Components::Collider>().lock();

    if (!tr || !rb || !scene || !cldr) { return; }
    if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive() || !cldr->GetActive()) { return; }

    const auto& up = tr->Up();
    const auto& center = tr->GetWorldPosition();
    const auto& down = -up;
    const auto& pos = tr->GetLocalPosition();
    bool        hit = false;

    const auto& octree = scene->GetObjectTree();
    octree.Iterate
      (
       pos, [&hit, &owner, &cldr, &center](const WeakObjectBase& obj)
       {
         if (const auto& locked = obj.lock())
         {
           if (locked == owner) { return false; }
           if (!GetCollisionDetector().IsCollisionLayer(owner->GetLayer(), locked->GetLayer())) { return false; }

           const auto& rcl = locked->GetComponent<Components::Collider>().lock();
           const auto& rtr = locked->GetComponent<Components::Transform>().lock();

           if (!rcl || !rtr) { return false; }
           if (!rcl->GetActive() || !rtr->GetActive()) { return false; }

           const auto& rowner = rcl->GetOwner().lock();
           if (!rowner) { return false; }
           if (const auto& parent = owner->GetParent().lock(); 
               rowner == parent) { return false; }

           // Check whether two objects are colliding in direction of down
           // Also check for position in y-axis so that it doesn't collide with ceiling
           if (Components::Collider::Intersects(cldr, rcl, Vector3::Down) &&
               center.y > rtr->GetWorldPosition().y)
           {
             hit = true;
             return true;
           }
         }

         return false;
       }
      );

    if (hit)
    {
      rb->SetGrounded(true);

      if (m_state_ == CHAR_STATE_JUMP || m_state_ == CHAR_STATE_FALL)
      {
        m_state_ = CHAR_STATE_IDLE;
      }
    }
  }
}
