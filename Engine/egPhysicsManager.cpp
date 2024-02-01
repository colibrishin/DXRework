#include "pch.h"
#include "egPhysicsManager.h"

#include "egBaseCollider.hpp"
#include "egCollision.h"
#include "egElastic.h"
#include "egFriction.h"
#include "egManagerHelper.hpp"
#include "egObject.hpp"
#include "egPhysics.hpp"
#include "egRigidbody.h"
#include "egSceneManager.hpp"
#include "egTransform.h"

namespace Engine::Manager::Physics
{
  void PhysicsManager::Initialize() {}

  void PhysicsManager::PreUpdate(const float& dt) {}

  void PhysicsManager::Update(const float& dt) {}

  void PhysicsManager::PreRender(const float& dt) {}

  void PhysicsManager::Render(const float& dt) {}

  void PhysicsManager::PostRender(const float& dt) {}

  void PhysicsManager::FixedUpdate(const float& dt)
  {
    if (const auto scene = GetSceneManager().GetActiveScene().lock())
    {
      const auto& rbs = scene->GetCachedComponents<Components::Rigidbody>();

      for (const auto rb : rbs)
      {
        UpdateGravity(rb.lock()->GetSharedPtr<Components::Rigidbody>().get());
        UpdateObject(rb.lock()->GetSharedPtr<Components::Rigidbody>().get(), dt);
      }
    }
  }

  void PhysicsManager::PostUpdate(const float& dt) {}

  void PhysicsManager::UpdateGravity(Components::Rigidbody* rb)
  {
    if (rb->IsFixed() || !rb->IsGravityAllowed()) { return; }

    const auto cl =
      rb->GetOwner().lock()->GetComponent<Components::Collider>().lock();

    if (!rb->IsGrounded()) { rb->AddForce(g_gravity_vec * cl->GetInverseMass()); }
    else { rb->AddForce(Vector3::Zero); }
  }

  void PhysicsManager::EpsilonGuard(Vector3& linear_momentum)
  {
    if (linear_momentum.x < g_epsilon && linear_momentum.x > -g_epsilon) { linear_momentum.x = 0.0f; }
    if (linear_momentum.y < g_epsilon && linear_momentum.y > -g_epsilon) { linear_momentum.y = 0.0f; }
    if (linear_momentum.z < g_epsilon && linear_momentum.z > -g_epsilon) { linear_momentum.z = 0.0f; }
  }

  void PhysicsManager::UpdateObject(Components::Rigidbody* rb, const float& dt)
  {
    if (rb->IsFixed()) { return; }

    const auto& cl =
      rb->GetOwner().lock()->GetComponent<Components::Collider>().lock();
    const auto t1 = rb->GetT1();

    Vector3 linear_momentum = rb->GetT1LinearVelocity(dt);
    const Vector3 linear_friction = Engine::Physics::EvalFriction
      (
       linear_momentum, rb->GetFrictionCoefficient(),
       dt
      );

    const Vector3 angular_momentum = rb->GetT1AngularVelocity(dt);

    linear_momentum += linear_friction;
    Engine::Physics::FrictionVelocityGuard(linear_momentum, linear_friction);

    const Vector3 drag_force = Engine::Physics::EvalDrag(linear_momentum, g_drag_coefficient);

    if (!rb->IsGrounded())
    {
      rb->SetDragForce(drag_force);
      linear_momentum += drag_force;
      Engine::Physics::FrictionVelocityGuard(linear_momentum, drag_force);
    }

    EpsilonGuard(linear_momentum);

    t1->SetLocalPosition(t1->GetLocalPosition() + linear_momentum);

    // Quaternion orientation = tr->GetRotation();
    // orientation += Quaternion{angular_momentum * dt * 0.5f, 0.0f} *
    // orientation; orientation.Normalize();
    // tr->SetRotation(orientation);

    rb->Reset();

    rb->SetT0LinearVelocity(linear_momentum);
    rb->SetT0AngularVelocity(angular_momentum);

    rb->SetLinearFriction(linear_friction);

    if (!rb->IsGrounded())
    {
      rb->SetDragForce(drag_force);
    }
  }
} // namespace Engine::Manager::Physics
