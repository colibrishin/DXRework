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
        UpdateObject(rb.lock()->GetSharedPtr<Components::Rigidbody>().get(), dt);
      }
    }
  }

  void PhysicsManager::PostUpdate(const float& dt) {}

  void PhysicsManager::EpsilonGuard(Vector3& lvel)
  {
    if (lvel.x < g_epsilon && lvel.x > -g_epsilon) { lvel.x = 0.0f; }
    if (lvel.y < g_epsilon && lvel.y > -g_epsilon) { lvel.y = 0.0f; }
    if (lvel.z < g_epsilon && lvel.z > -g_epsilon) { lvel.z = 0.0f; }
  }

  void PhysicsManager::UpdateObject(Components::Rigidbody* rb, const float& dt)
  {
    if (rb->IsFixed()) { return; }

    const auto& cl =
      rb->GetOwner().lock()->GetComponent<Components::Collider>().lock();
    const auto t1 = rb->GetT1();

    Vector3 lvel = rb->GetT0LinearVelocity();

    const Vector3 lfrc = Engine::Physics::EvalFriction
      (
       lvel, rb->GetFrictionCoefficient(),
       dt
      );

    const Vector3 rvel = rb->GetT0AngularVelocity();

    lvel += lfrc;
    Engine::Physics::FrictionVelocityGuard(lvel, lfrc);

    EpsilonGuard(lvel);

    t1->SetLocalPosition(t1->GetLocalPosition() + (lvel * dt) + (rb->GetT0Force() * (dt * dt * 0.5f)));

    if (!rb->GetNoAngular())
    {
      Quaternion orientation = t1->GetLocalRotation();
      orientation += Quaternion{rvel * dt * 0.5f, 0.0f} * orientation;
      orientation.Normalize();
      t1->SetLocalRotation(orientation);
      rb->SetT0AngularVelocity(rvel + (rb->GetT0Torque() + rb->GetT1Torque()) * (dt * 0.5f));
    }

    rb->SetT0LinearVelocity(lvel + (rb->GetT0Force() + rb->GetT1Force()) * (dt * 0.5f));

    rb->Reset();

    rb->SetLinearFriction(lfrc);
  }
} // namespace Engine::Manager::Physics
