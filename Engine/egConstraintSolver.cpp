#include "pch.h"
#include "egConstraintSolver.h"

#include "egBaseCollider.hpp"
#include "egCollisionDetector.h"
#include "egGlobal.h"
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
        auto& infos = GetCollisionDetector().GetCollisionInfo();

        static tbb::affinity_partitioner ap;

        for (const auto& info : infos)
        {
            if (info.collision)
            {
                ResolveCollision(info.lhs, info.rhs);
            }
        }

        infos.clear();
        m_collision_resolved_set_.clear();
    }

    void ConstraintSolver::PreRender(const float& dt) {}

    void ConstraintSolver::Render(const float& dt) {}

    void ConstraintSolver::PostRender(const float& dt) {}

    void ConstraintSolver::FixedUpdate(const float& dt) {}

    void ConstraintSolver::PostUpdate(const float& dt) {}

    void ConstraintSolver::ResolveCollision(const WeakObject& lhs, const WeakObject& rhs)
    {
        auto rb = lhs.lock()->GetComponent<Components::Rigidbody>().lock();
        auto tr = lhs.lock()->GetComponent<Components::Transform>().lock();

        auto rb_other = rhs.lock()->GetComponent<Components::Rigidbody>().lock();
        auto tr_other = rhs.lock()->GetComponent<Components::Transform>().lock();

        if (rb && rb_other)
        {
            if (m_collision_resolved_set_.contains({lhs.lock()->GetID(), rhs.lock()->GetID()}))
            {
                return;
            }

            m_collision_resolved_set_.insert({lhs.lock()->GetID(), rhs.lock()->GetID()});
            m_collision_resolved_set_.insert({rhs.lock()->GetID(), lhs.lock()->GetID()});

            if (rb->IsFixed() && rb_other->IsFixed())
            {
                return;
            }

            if (rb->IsFixed())
            {
                std::swap(rb, rb_other);
                std::swap(tr, tr_other);
            }

            const auto cl       = lhs.lock()->GetComponent<Components::Collider>().lock();
            const auto cl_other = rhs.lock()->GetComponent<Components::Collider>().lock();

            if (!cl || !cl_other)
            {
                return;
            }

            Vector3 linear_vel;
            Vector3 angular_vel;

            Vector3 other_linear_vel;
            Vector3 other_angular_vel;

            const Vector3 pos       = tr->GetWorldPosition();
            const Vector3 other_pos = tr_other->GetWorldPosition();

            Vector3 lhs_normal;
            float   lhs_pen;

            if (!cl->GetPenetration(*cl_other, lhs_normal, lhs_pen))
            {
                return;
            }

            // Gets the vector from center to collision point.
            const auto lbnd = cl->GetBounding();
            const auto rbnd = cl_other->GetBounding();

            float distance = 0;
            if (!lbnd.TestRay(rbnd, lhs_normal, distance))
            {
                return;
            }

            Vector3 collision_point = pos - (lhs_normal * distance);
            Vector3 lhs_weight_pen;
            Vector3 rhs_weight_pen;

            Engine::Physics::EvalImpulse(
                                         pos, other_pos, collision_point, lhs_pen, lhs_normal, cl->GetInverseMass(),
                                         cl_other->GetInverseMass(), rb->GetAngularMomentum(),
                                         rb_other->GetAngularMomentum(), rb->GetLinearMomentum(),
                                         rb_other->GetLinearMomentum(), cl->GetInertiaTensor(),
                                         cl_other->GetInertiaTensor(), linear_vel, other_linear_vel, angular_vel,
                                         other_angular_vel, lhs_weight_pen, rhs_weight_pen);

            // Fast collision penalty
            const auto cps     = static_cast<float>(cl->GetCPS(rhs.lock()->GetID()));
            const auto fps     = static_cast<float>(GetApplication().GetFPS());
            auto       cps_val = cps / fps;

            // Not collided object is given to solver.
            if (cps == 0) throw std::logic_error("Collision count is zero");
            if (!isfinite(cps)) cps_val = 0.f; // where fps is zero

            // Accumulated collision penalty
            const auto collision_count = cl->GetCollisionCount(rhs.lock()->GetID());
            const auto log_count       = std::clamp(std::powf(2, collision_count), 2.f, (float)g_energy_reduction_ceil);
            const auto penalty         = log_count / (float)g_energy_reduction_ceil;
            const auto penalty_sum     = std::clamp(cps_val + penalty, 0.f, 1.f);
            const auto reduction       = 1.0f - penalty_sum;

            if (!rb->IsFixed())
            {
                tr->SetWorldPosition(pos + lhs_weight_pen);
                rb->SetLinearMomentum(rb->GetLinearMomentum() - (linear_vel * reduction));
                rb->SetAngularMomentum(rb->GetAngularMomentum() - (angular_vel * reduction));
            }

            if (!rb_other->IsFixed())
            {
                tr_other->SetWorldPosition(pos + rhs_weight_pen);
                rb_other->SetLinearMomentum(rb_other->GetLinearMomentum() + (linear_vel * reduction));
                rb_other->SetAngularMomentum(rb_other->GetAngularMomentum() + (angular_vel * reduction));
            }
        }
    }
} // namespace Engine::Manager::Physics
