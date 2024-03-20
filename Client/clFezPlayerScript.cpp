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
    case CHAR_STATE_IDLE: UpdateMove();
      UpdateRotate();
      UpdateJump();
      break;
    case CHAR_STATE_WALK: UpdateMove();
      UpdateJump();
      break;
    case CHAR_STATE_RUN: break;
    case CHAR_STATE_JUMP: UpdateMove();
      break;
    case CHAR_STATE_CLIMB: break;
    case CHAR_STATE_SWIM: break;
    case CHAR_STATE_ROTATE: UpdateRotate();
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

  SCRIPT_CLONE_IMPL(FezPlayerScript)

  FezPlayerScript::FezPlayerScript()
    : Script(SCRIPT_T_FEZ_PLAYER, {}),
      m_state_(CHAR_STATE_IDLE),
      m_prev_state_(CHAR_STATE_IDLE),
      m_rotation_count_(0) { }

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
    const auto& key_state = GetApplication().GetCurrentKeyState();
    constexpr float speed = 1.f;
    bool moving = false;

    if (key_state.IsKeyDown(Keyboard::D))
    {
      rb->AddT1Force(right * speed);
      moving = true;
    }
    if (key_state.IsKeyDown(Keyboard::A))
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

  void FezPlayerScript::UpdateRotate()
  {
    if (GetOwner().expired()) { return; }

    static const Quaternion rotations[4] = 
    {
      Quaternion::CreateFromAxisAngle(Vector3::Up, 0.0f),
      Quaternion::CreateFromAxisAngle(Vector3::Up, XMConvertToRadians(90.0f)),
      Quaternion::CreateFromAxisAngle(Vector3::Up, XMConvertToRadians(180.0f)),
      Quaternion::CreateFromAxisAngle(Vector3::Up, XMConvertToRadians(270.0f))
    };

    const auto& owner = GetOwner().lock();
    const auto& tr    = owner->GetComponent<Components::Transform>().lock();
    const auto& rb    = owner->GetComponent<Components::Rigidbody>().lock();

    if (!tr || !rb) { return; }
    if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive()) { return; }

    const auto  rot       = tr->GetLocalRotation();
    bool        rotating  = false;
    const auto  angle     = Quaternion::Angle(rot, Quaternion::CreateFromAxisAngle(Vector3::Right, 0.f));

    // due to the slerp, rotation is not exact.
    if (tr->GetLocalRotation() != rotations[m_rotation_count_])
    {
      tr->SetLocalRotation(rotations[m_rotation_count_]);
    }

    if (GetApplication().HasKeyChanged(Keyboard::Q))
    {
      m_rotation_count_ = (m_rotation_count_ + 1) % 4;
      tr->SetLocalRotation(rotations[m_rotation_count_]);
      rotating = true;
    }
    if (GetApplication().HasKeyChanged(Keyboard::E))
    {
      m_rotation_count_ = (m_rotation_count_ + 3) % 4;
      tr->SetLocalRotation(rotations[m_rotation_count_]);
      rotating = true;
    }

    if (rotating || tr->GetLocalRotation() != Quaternion::Identity)
    {
      m_state_ = CHAR_STATE_ROTATE;
      rb->FullReset();
      rb->SetFixed(true);
    }
    else if (m_state_ == CHAR_STATE_ROTATE)
    {
      m_state_ = CHAR_STATE_IDLE;
      // Clear accumulated forces (e.g., collision reaction force) and set fixed to false
      rb->FullReset();
      rb->SetFixed(false);
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

    const auto&     up         = tr->Up();
    constexpr float jump_force = 400.f;
    constexpr float jump_apex = 5.f;

    if (GetApplication().HasKeyChanged(Keyboard::Space) || GetApplication().HasKeyChanged(Keyboard::W))
    {
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
