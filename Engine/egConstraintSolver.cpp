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
        const auto rb = lhs.lock()->GetComponent<Components::Rigidbody>().lock();
        const auto tr = lhs.lock()->GetComponent<Components::Transform>().lock();

        const auto rb_other = rhs.lock()->GetComponent<Components::Rigidbody>().lock();
        const auto tr_other = rhs.lock()->GetComponent<Components::Transform>().lock();

        if (rb && rb_other)
        {
            if (m_collision_resolved_set_.contains({lhs.lock()->GetID(), rhs.lock()->GetID()}))
            {
                return;
            }

            m_collision_resolved_set_.insert({lhs.lock()->GetID(), rhs.lock()->GetID()});
            m_collision_resolved_set_.insert({rhs.lock()->GetID(), lhs.lock()->GetID()});

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

            Vector3 normal;
            float   penetration;

            cl->GetPenetration(*cl_other, normal, penetration);
            const Vector3 point = pos + normal * penetration;

            Vector3 lhs_penetration;
            Vector3 rhs_penetration;

            Engine::Physics::EvalImpulse(
                                         pos, other_pos, point, penetration, normal, cl->GetInverseMass(),
                                         cl_other->GetInverseMass(), rb->GetAngularMomentum(),
                                         rb_other->GetAngularMomentum(), rb->GetLinearMomentum(),
                                         rb_other->GetLinearMomentum(), cl->GetInertiaTensor(),
                                         cl_other->GetInertiaTensor(), linear_vel, other_linear_vel, angular_vel,
                                         other_angular_vel, lhs_weight_pen, rhs_weight_pen);


            const auto collided_count = cl->GetCollisionCount(rhs.lock()->GetID());
            const auto fps            = GetApplication().GetFPS();

            auto ratio = static_cast<float>(collided_count) / static_cast<float>(fps);
            ratio      = std::clamp(ratio, 0.f, 1.f);

            if (!std::isfinite(ratio))
            {
                ratio = 0.0f;
            }

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

            // Assuming lhs rigid-body is the movable object.
            tr->SetWorldPosition(pos + lhs_penetration);
            rb->SetLinearMomentum(linear_vel * reduction);
            rb->SetAngularMomentum(angular_vel * reduction);

            if (!rb_other->IsFixed())
            {
                tr_other->SetWorldPosition(other_pos + rhs_penetration);

                rb_other->SetLinearMomentum(
                                            other_linear_vel * reduction);
                rb_other->SetAngularMomentum(
                                             other_angular_vel * reduction);
            }
        }
    }
} // namespace Engine::Manager::Physics
