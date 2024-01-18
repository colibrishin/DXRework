#include "pch.h"
#include "clCharacterController.hpp"
#include "client.h"

#include <egApplication.h>
#include <egCamera.h>
#include <egCollisionDetector.h>
#include <egDebugger.hpp>
#include <egRigidbody.h>
#include <egTransform.h>
#include <boost/serialization/export.hpp>

#include "clPlayer.h"
#include "egBaseCollider.hpp"
#include "egMouseManager.h"
#include "egSceneManager.hpp"

SERIALIZER_ACCESS_IMPL
(
 Client::State::CharacterController,
 _ARTAG(_BSTSUPER(Engine::Components::StateController))
 _ARTAG(m_shoot_interval) _ARTAG(m_hp_)
)

namespace Client::State
{
  void CharacterController::Initialize()
  {
    SetState(CHAR_STATE_IDLE);
    const auto cam = GetOwner().lock()->GetScene().lock()->GetMainCamera().lock();

    if (cam)
    {
      m_head_ = GetOwner().lock()->GetSharedPtr<Object::Player>()->GetHead().lock();

      if (const auto head = m_head_.lock())
      {
        m_head_ = GetOwner().lock()->GetSharedPtr<Object::Player>()->GetHead().lock();
        m_head_.lock()->AddChild(cam);
      }
    }
  }

  void CharacterController::PreUpdate(const float& dt)
  {
    StateController::PreUpdate(dt);
    CheckGround();
  }

  void CharacterController::PostUpdate(const float& dt) { StateController::PostUpdate(dt); }

  void CharacterController::CheckJump(
    const boost::shared_ptr<Components::Rigidbody>& rb
  )
  {
    if (!rb->GetGrounded()) { SetState(CHAR_STATE_JUMP); }
    else { SetState(CHAR_STATE_IDLE); }
  }

  void CharacterController::CheckMove(
    const boost::shared_ptr<Components::Rigidbody>& rb
  )
  {
    float      speed = 1.0f;
    const auto scene = GetSceneManager().GetActiveScene().lock();

    auto forward = GetOwner().lock()->GetComponent<Components::Transform>().lock()->Forward();
    auto ortho   =
      Vector3::Transform
      (
       forward,
       Matrix::CreateRotationY(-XMConvertToRadians(90.0f))
      ) * speed;

    forward *= {1.f, 0.f, 1.f};
    ortho *= {1.f, 0.f, 1.f};

    bool pressed = false;

    if (GetApplication().GetKeyState().IsKeyDown(Keyboard::W))
    {
      rb->AddForce(forward);
      pressed = true;
    }

    if (GetApplication().GetKeyState().IsKeyDown(Keyboard::A))
    {
      rb->AddForce(ortho);
      pressed = true;
    }

    if (GetApplication().GetKeyState().IsKeyDown(Keyboard::S))
    {
      rb->AddForce(-forward);
      pressed = true;
    }

    if (GetApplication().GetKeyState().IsKeyDown(Keyboard::D))
    {
      rb->AddForce(-ortho);
      pressed = true;
    }

    if (!pressed) { SetState(CHAR_STATE_IDLE); }
    else { SetState(CHAR_STATE_WALK); }
  }

  bool CharacterController::CheckAttack(const float& dt)
  {
    if (GetApplication().GetMouseState().leftButton)
    {
      SetState(CHAR_STATE_ATTACK);

      if (m_shoot_interval < 0.5f)
      {
        m_shoot_interval += dt;
        return false;
      }

      m_shoot_interval = 0.f;
      const auto tr    =
        GetOwner().lock()->GetComponent<Components::Transform>().lock();

      Ray ray;
      ray.position  = tr->GetWorldPosition();
      ray.direction = tr->Forward();

      constexpr float distance = 5.f;

      GetDebugger().Draw(ray, Colors::AliceBlue);
      std::vector<WeakObject> out;

      if (GetCollisionDetector().Hitscan
        (
         ray.position, distance, ray.direction, out
        )) { return true; }
    }

    return false;
  }

  void CharacterController::CheckGround() const
  {
    const auto  scene = GetOwner().lock()->GetScene().lock();
    const auto& tree  = scene->GetObjectTree();
    const auto  rb    = GetOwner().lock()->GetComponent<Components::Rigidbody>().lock();

    std::queue<const Octree*> q;
    q.push(&tree);

    while (!q.empty())
    {
      const auto node = q.front();
      q.pop();

      const auto& value    = node->Read();
      const auto& children = node->Next();

      for (const auto v : value)
      {
        const auto lcl = GetOwner().lock()->GetComponent<Components::Collider>().lock();
        const auto rcl = v.lock()->GetComponent<Components::Collider>().lock();

        if (!GetCollisionDetector().IsCollisionLayer(GetOwner().lock()->GetLayer(), v.lock()->GetLayer())) { continue; }

        if (!rcl || lcl == rcl) { continue; }

        if (Components::Collider::Intersects(lcl, rcl, Vector3::Down))
        {
          rb->SetGrounded(true);
          return;
        }
      }

      for (const auto& child : children)
      {
        if (child &&
            child->Contains
            (GetOwner().lock()->GetComponent<Components::Transform>().lock()->GetWorldPosition())) { q.push(child); }
      }
    }
  }

  void CharacterController::Update(const float& dt)
  {
    const auto rb = GetOwner().lock()->GetComponent<Components::Rigidbody>().lock();

    if (!rb) { return; }

    if (const auto head = m_head_.lock())
    {
      const auto head_tr = m_head_.lock()->GetComponent<Components::Transform>().lock();
      const auto mouse_y = GetMouseManager().GetMouseYRotation();
      head_tr->SetLocalRotation(mouse_y);
    }

    const auto body_tr = GetOwner().lock()->GetComponent<Components::Transform>().lock();
    const auto mouse_x = GetMouseManager().GetMouseXRotation();

    body_tr->SetLocalRotation(mouse_x);

    CheckJump(rb);
    CheckMove(rb);
    CheckAttack(dt);

    switch (GetState<eCharacterState>())
    {
    case CHAR_STATE_IDLE: if (HasStateChanged()) { GetDebugger().Log("Idle"); }
      break;
    case CHAR_STATE_WALK: if (HasStateChanged()) { GetDebugger().Log("Walk"); }
      break;
    case CHAR_STATE_RUN: break;
    case CHAR_STATE_JUMP: if (HasStateChanged()) { GetDebugger().Log("Jump"); }
      break;
    case CHAR_STATE_ATTACK: if (HasStateChanged()) { GetDebugger().Log("Attack"); }
      break;
    case CHAR_STATE_DIE: break;
    case CHAR_STATE_HIT: break;
    case CHAR_STATE_MAX:
    default: break;
    }
  }

  void CharacterController::FixedUpdate(const float& dt) {}
} // namespace Client::State
