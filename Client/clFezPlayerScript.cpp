#include "pch.h"
#include "clFezPlayerScript.h"

#include "egCamera.h"
#include "egObjectBase.hpp"
#include "egRigidbody.h"
#include "egTransform.h"

SERIALIZE_IMPL
(
 Client::Scripts::FezPlayerScript,
 _ARTAG(_BSTSUPER(Engine::Script))
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
    if (m_prev_state_ == m_state_)
    {
      switch (m_state_)
      {
      case CHAR_STATE_IDLE: 
          UpdateMove();
          UpdateRotate();
          break;
      case CHAR_STATE_WALK: 
          UpdateMove();
          break;
      case CHAR_STATE_RUN: break;
      case CHAR_STATE_JUMP: 
          UpdateMove();
          break;
      case CHAR_STATE_CLIMB: break;
      case CHAR_STATE_SWIM: break;
      case CHAR_STATE_ROTATE:
          UpdateRotate();
          break;
      case CHAR_STATE_FALL: break;
      case CHAR_STATE_ATTACK: break;
      case CHAR_STATE_HIT: break;
      case CHAR_STATE_DIE: break;
      case CHAR_STATE_MAX: break;
      default: ;
      }
    }

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
      m_prev_state_(CHAR_STATE_IDLE) { }

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
    const auto& key_state = GetApplication().GetKeyState();
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

    const auto& owner = GetOwner().lock();
    const auto& tr    = owner->GetComponent<Components::Transform>().lock();
    const auto& rb    = owner->GetComponent<Components::Rigidbody>().lock();

    if (!tr || !rb) { return; }
    if (!owner->GetActive() || !tr->GetActive() || !rb->GetActive()) { return; }

    const auto& key_state = GetApplication().GetKeyState();
    const auto  rot       = tr->GetLocalRotation();
    bool        rotating  = false;

    if (key_state.IsKeyDown(Keyboard::Q))
    {
      tr->SetLocalRotation(rot * Quaternion::CreateFromAxisAngle(Vector3::Up, -90.0f));
      rotating = true;
    }
    if (key_state.IsKeyDown(Keyboard::E))
    {
      tr->SetLocalRotation(rot * Quaternion::CreateFromAxisAngle(Vector3::Up, 90.0f));
      rotating = true;
    }

    if (rotating || tr->GetLocalRotation() != Quaternion::Identity)
    {
      m_state_ = CHAR_STATE_ROTATE;
      rb->SetFixed(true);
      rb->AddT1Force(-rb->GetT1Force());
    }
    else
    {
      m_state_ = CHAR_STATE_IDLE;
      rb->SetFixed(false);
    }
  }
}
