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

    if (const auto player = GetOwner().lock()->GetSharedPtr<Object::Player>())
    {
      m_head_ = player->GetHead();
    }

    MoveCameraToChild(true);
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
         GetOwner(), ray.position, distance, ray.direction, out
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

    if (const auto head = m_head_.lock() && !m_top_view_)
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

  void CharacterController::MoveCameraToChild(bool active)
  {
    if (const auto head = m_head_.lock(); 
        const auto scene = GetOwner().lock()->GetScene().lock())
    {
      const auto cam = scene->GetMainCamera().lock();

      if (head && scene && cam && !active)
      {
        head->DetachChild(m_cam_id_);
        m_cam_id_ = g_invalid_id;
      }
      else if (head && scene && cam && active)
      {
        head->AddChild(cam);
        m_cam_id_ = cam->GetLocalID();
      }
    }
  }

  void CharacterController::SetActive(bool active)
  {
    MoveCameraToChild(active);
    StateController::SetActive(active);
  }

  void CharacterController::SetHeadView(const bool head_view)
  {
    if (m_cam_id_ == g_invalid_id) { return; }

    const auto head = m_head_.lock();

    if (!head) { return; }

    const auto cam_obj = head->GetChild(m_cam_id_).lock();

    if (!cam_obj) { return; }

    const auto cam = cam_obj->GetSharedPtr<Objects::Camera>();
    const auto cam_tr = cam_obj->GetComponent<Components::Transform>().lock();

    if (m_top_view_)
    {
      cam->SetOrthogonal(false);
      cam_tr->SetLocalPosition({0.f, 10.f, 0.f});
      cam_tr->SetLocalRotation(Quaternion::CreateFromAxisAngle(Vector3::Right, XM_PIDIV2));
    }
    else
    {
      cam->SetOrthogonal(false);
      cam_tr->SetLocalPosition(Vector3::Zero);
      cam_tr->SetLocalRotation(Quaternion::Identity);
    }

    m_top_view_ = head_view;
  }

  void CharacterController::Hit(const float damage)
  {
    m_hp_ -= damage;

    if (m_hp_ <= 0.f)
    {
      SetState(CHAR_STATE_DIE);
    }
    else
    {
      SetState(CHAR_STATE_HIT);
    }
  }
} // namespace Client::State
