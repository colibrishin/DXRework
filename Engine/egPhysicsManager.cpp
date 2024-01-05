#include "pch.h"
#include "egPhysicsManager.h"

#include "egBaseCollider.hpp"
#include "egCollision.h"
#include "egElastic.h"
#include "egFriction.h"
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
        if (rb->IsFixed() || !rb->IsGravityAllowed())
        {
            return;
        }

        const auto cl =
                rb->GetOwner().lock()->GetComponent<Components::Collider>().lock();

        if (!rb->IsGrounded())
        {
            rb->AddForce(Engine::g_gravity_vec * cl->GetInverseMass());
        }
        else
        {
            rb->AddForce(Vector3::Zero);
        }
    }

    void PhysicsManager::EpsilonGuard(Vector3& linear_momentum)
    {
        if (linear_momentum.Length() < g_epsilon)
        {
            linear_momentum = Vector3::Zero;
        }
    }

    void PhysicsManager::UpdateObject(Components::Rigidbody* rb, const float& dt)
    {
        if (rb->IsFixed())
        {
            return;
        }

        const auto& cl = 
            rb->GetOwner().lock()->GetComponent<Components::Collider>().lock();
        const auto& tr =
            rb->GetOwner().lock()->GetComponent<Components::Transform>().lock();

        float mass = 1.f;

        if (cl)
        {
            mass = cl->GetInverseMass();
        }

        Vector3 linear_momentum =
                rb->GetLinearMomentum() + (rb->GetForce() * mass * dt);
        const Vector3 linear_friction = Engine::Physics::EvalFriction(
                                                                      linear_momentum, rb->GetFrictionCoefficient(),
                                                                      dt);

        const Vector3 angular_momentum =
                rb->GetAngularMomentum() + rb->GetTorque() * mass * dt;

        rb->SetLinearFriction(linear_friction);
        linear_momentum += linear_friction;
        Engine::Physics::FrictionVelocityGuard(linear_momentum, linear_friction);

        if (!rb->IsGrounded())
        {
            const Vector3 drag_force = Engine::Physics::EvalDrag(linear_momentum, g_drag_coefficient);
            rb->SetDragForce(drag_force);
            linear_momentum += drag_force;
            Engine::Physics::FrictionVelocityGuard(linear_momentum, drag_force);
        }

        EpsilonGuard(linear_momentum);

        rb->SetLinearMomentum(linear_momentum);
        rb->SetAngularMomentum(angular_momentum);

        tr->SetLocalPosition(tr->GetLocalPosition() + linear_momentum);

        // Quaternion orientation = tr->GetRotation();
        // orientation += Quaternion{angular_momentum * dt * 0.5f, 0.0f} *
        // orientation; orientation.Normalize();

        // tr->SetRotation(orientation);

        rb->Reset();
    }
} // namespace Engine::Manager::Physics
