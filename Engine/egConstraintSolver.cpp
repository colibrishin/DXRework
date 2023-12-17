#include "pch.h"
#include "egConstraintSolver.h"
#include "egCollisionDetector.h"
#include "egObject.hpp"
#include "egSceneManager.hpp"
#include "egTransform.h"
#include "egRigidbody.h"

namespace Engine::Manager::Physics
{
    void ConstraintSolver::Initialize() {}

    void ConstraintSolver::PreUpdate(const float& dt) {}

    void ConstraintSolver::Update(const float& dt)
    {
        const auto scene = GetSceneManager().GetActiveScene().lock();

        const auto& rbs = scene->GetCachedComponents<Component::Rigidbody>();

        for (const auto& rb : rbs)
        {
            if (const auto locked = rb.lock())
            {
                if (g_speculation_enabled)
                {
                    CheckSpeculation(*locked->GetOwner().lock());
                }
                CheckCollision(*locked->GetOwner().lock());
            }
        }

        m_collision_resolved_set_.clear();
        m_speculative_resolved_set_.clear();
    }

    void ConstraintSolver::PreRender(const float& dt) {}

    void ConstraintSolver::Render(const float& dt) {}

    void ConstraintSolver::PostRender(const float& dt) {}

    void ConstraintSolver::FixedUpdate(const float& dt) {}

    void ConstraintSolver::CheckCollision(Abstract::Object& obj)
    {
        const auto cl = obj.GetComponent<Component::Collider>().lock();

        if (!cl)
        {
            return;
        }

        const auto others = cl->GetCollidedObjects();

        for (const auto& id : others)
        {
            const auto scene = GetSceneManager().GetActiveScene().lock();

            const auto other = scene->FindGameObject(id).lock();

            if (!other)
            {
                continue;
            }

            if (GetCollisionDetector().IsCollidedInFrame(obj.GetID(), other->GetID()))
            {
                ResolveCollision(obj, *other);
            }
        }
    }

    void ConstraintSolver::CheckSpeculation(Abstract::Object& obj)
    {
        const auto cl = obj.GetComponent<Component::Collider>().lock();

        if (!cl)
        {
            return;
        }

        const auto others = cl->GetSpeculation();

        for (const auto& id : others)
        {
            const auto scene = GetSceneManager().GetActiveScene().lock();

            const auto other = scene->FindGameObject(id).lock();

            if (!other)
            {
                continue;
            }

            ResolveSpeculation(obj, *other);
        }
    }

    void ConstraintSolver::ResolveCollision(
        Abstract::Object& lhs,
        Abstract::Object& rhs)
    {
        const auto rb = lhs.GetComponent<Component::Rigidbody>().lock();
        const auto tr = lhs.GetComponent<Component::Transform>().lock();
        // Main collider is considered as the collider that wraps the object.

        const auto rb_other = rhs.GetComponent<Component::Rigidbody>().lock();
        const auto tr_other = rhs.GetComponent<Component::Transform>().lock();

        if (rb && rb_other)
        {
            if (rb->IsFixed())
            {
                return;
            }

            if (m_collision_resolved_set_.contains({lhs.GetID(), rhs.GetID()}) ||
                m_collision_resolved_set_.contains({rhs.GetID(), lhs.GetID()}))
            {
                return;
            }

            const auto cl       = rb->GetMainCollider().lock();
            const auto cl_other = rb_other->GetMainCollider().lock();

            if (!cl || !cl_other)
            {
                return;
            }

            Vector3 linear_vel;
            Vector3 angular_vel;

            Vector3 other_linear_vel;
            Vector3 other_angular_vel;

            const Vector3 pos       = cl->GetPosition();
            const Vector3 other_pos = cl_other->GetPosition();

            const Vector3 delta = (other_pos - pos);
            Vector3       dir;
            delta.Normalize(dir);

            Vector3 normal;
            float   penetration;

            // Calculate the penetration and the normal with main collider which is the
            // wrapper of the object.
            cl->GetPenetration(*cl_other, normal, penetration);
            const Vector3 point = cl->GetPosition() + normal * penetration;

            Vector3 lhs_penetration;
            Vector3 rhs_penetration;

            Engine::Physics::EvalImpulse(
                                         pos, other_pos, point, penetration, normal, cl->GetInverseMass(),
                                         cl_other->GetInverseMass(), rb->GetAngularMomentum(),
                                         rb_other->GetAngularMomentum(), rb->GetLinearMomentum(),
                                         rb_other->GetLinearMomentum(), cl->GetInertiaTensor(),
                                         cl_other->GetInertiaTensor(), linear_vel, other_linear_vel, angular_vel,
                                         other_angular_vel, lhs_penetration, rhs_penetration);

            tr->SetPosition(pos + lhs_penetration);

            // Apply the changes to the every colliders.
            for (const auto& child : lhs.GetComponents<Component::Collider>())
            {
                if (const auto locked = child.lock())
                {
                    locked->SetPosition(locked->GetPosition() + lhs_penetration);
                }
            }

            const auto collided_count = cl->GetCollisionCount(rhs.GetID());
            const auto fps            = GetApplication().GetFPS();

            auto ratio = static_cast<float>(collided_count) / static_cast<float>(fps);

            if (collided_count > fps)
            {
                ratio = 0.0f;
            }

            ratio                = std::clamp(ratio, 0.f, 1.f);
            const auto ratio_inv = 1.0f - ratio;

            const auto collision_reduction = std::powf(
                                                       static_cast<float>(collided_count),
                                                       static_cast<float>(g_collision_energy_reduction_multiplier.
                                                           load()));

            auto reduction = (ratio_inv / collision_reduction);

            // nan guard
            if (!isfinite(reduction))
            {
                reduction = ratio_inv;
            }

            rb->SetLinearMomentum(linear_vel * reduction);
            rb->SetAngularMomentum(angular_vel * reduction);

            if (!rb_other->IsFixed())
            {
                tr_other->SetPosition(other_pos + rhs_penetration);

                for (const auto& child : rhs.GetComponents<Component::Collider>())
                {
                    if (const auto locked = child.lock())
                    {
                        locked->SetPosition(locked->GetPosition() + rhs_penetration);
                    }
                }

                rb_other->SetLinearMomentum(
                                            other_linear_vel * reduction);
                rb_other->SetAngularMomentum(
                                             other_angular_vel * reduction);
            }

            m_collision_resolved_set_.insert({lhs.GetID(), rhs.GetID()});
            m_collision_resolved_set_.insert({rhs.GetID(), lhs.GetID()});
        }
    }

    void ConstraintSolver::ResolveSpeculation(
        Abstract::Object& lhs,
        Abstract::Object& rhs)
    {
        const auto rb = lhs.GetComponent<Component::Rigidbody>().lock();
        const auto tr = lhs.GetComponent<Component::Transform>().lock();

        const auto rb_other = rhs.GetComponent<Component::Rigidbody>().lock();
        const auto tr_other = rhs.GetComponent<Component::Transform>().lock();

        if (rb && tr && rb_other && tr_other)
        {
            if (m_speculative_resolved_set_.contains({lhs.GetID(), rhs.GetID()}) ||
                m_speculative_resolved_set_.contains({rhs.GetID(), lhs.GetID()}))
            {
                return;
            }

            const auto cl       = rb->GetMainCollider().lock();
            const auto cl_other = rb_other->GetMainCollider().lock();

            if (!cl || !cl_other)
            {
                return;
            }

            Ray ray{};
            ray.position        = cl->GetPreviousPosition();
            const auto velocity = rb->GetLinearMomentum();
            velocity.Normalize(ray.direction);

            const auto length                = velocity.Length();
            float      intersection_distance = 0.0f;

            // Forward collision check
            if (cl_other->Intersects(ray, length, intersection_distance))
            {
                const Vector3 minimum_penetration = ray.direction * intersection_distance;
                tr->SetPosition(tr->GetPosition() - minimum_penetration);

                for (const auto& child : lhs.GetComponents<Component::Collider>())
                {
                    if (const auto locked = child.lock())
                    {
                        locked->SetPosition(locked->GetPosition() - minimum_penetration);
                        GetDebugger().Draw(
                                           locked->GetPreviousPosition(),
                                           locked->GetPosition(), DirectX::Colors::Cyan);
                    }
                }

                m_speculative_resolved_set_.insert({lhs.GetID(), rhs.GetID()});
                m_speculative_resolved_set_.insert({rhs.GetID(), lhs.GetID()});
            }

            cl->RemoveSpeculationObject(rhs.GetID());
            cl_other->RemoveSpeculationObject(lhs.GetID());
        }
    }
} // namespace Engine::Manager::Physics
