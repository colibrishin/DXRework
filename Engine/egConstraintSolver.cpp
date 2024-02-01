#include "pch.h"
#include "egConstraintSolver.h"

#include "egBaseCollider.hpp"
#include "egCollisionDetector.h"
#include "egGlobal.h"
#include "egManagerHelper.hpp"
#include "egObject.hpp"
#include "egRigidbody.h"
#include "egSceneManager.hpp"
#include "egTransform.h"

namespace Engine::Manager::Physics
{
  void ConstraintSolver::Initialize() {}

  void ConstraintSolver::PreUpdate(const float& dt) {}

  void ConstraintSolver::Update(const float& dt) {}

  void ConstraintSolver::PreRender(const float& dt) {}

  void ConstraintSolver::Render(const float& dt) {}

  void ConstraintSolver::PostRender(const float& dt) {}

  void ConstraintSolver::FixedUpdate(const float& dt)
  {
    auto& infos = GetCollisionDetector().GetCollisionInfo();

    static tbb::affinity_partitioner ap;

    for (const auto& info : infos)
    {
      if (info.speculative) { ResolveSpeculation(info.lhs, info.rhs); }
      if (info.collision) { ResolveCollision(info.lhs, info.rhs); }
    }

    infos.clear();
    m_collision_resolved_set_.clear();
  }

  void ConstraintSolver::PostUpdate(const float& dt) {}

  void ConstraintSolver::ResolveCollision(const WeakObject& p_lhs, const WeakObject& p_rhs)
  {
    auto lhs = p_lhs;
    auto rhs = p_rhs;

    auto rb = lhs.lock()->GetComponent<Components::Rigidbody>().lock();
    auto lt0 = lhs.lock()->GetComponent<Components::Transform>().lock();

    auto rb_other = rhs.lock()->GetComponent<Components::Rigidbody>().lock();
    auto rt0 = rhs.lock()->GetComponent<Components::Transform>().lock();

    if (rb && rb_other)
    {
      if (m_collision_resolved_set_.contains({lhs.lock()->GetID(), rhs.lock()->GetID()})) { return; }

      m_collision_resolved_set_.insert({lhs.lock()->GetID(), rhs.lock()->GetID()});
      m_collision_resolved_set_.insert({rhs.lock()->GetID(), lhs.lock()->GetID()});

      if (rb->IsFixed() && rb_other->IsFixed()) { return; }

      auto cl       = lhs.lock()->GetComponent<Components::Collider>().lock();
      auto cl_other = rhs.lock()->GetComponent<Components::Collider>().lock();

      if (rb->IsFixed())
      {
        std::swap(rb, rb_other);
        std::swap(lt0, rt0);
        std::swap(lhs, rhs);
        std::swap(cl, cl_other);
      }

      if (!cl || !cl_other) { return; }

      Vector3 linear_vel;
      Vector3 angular_vel;

      Vector3 other_linear_vel;
      Vector3 other_angular_vel;

      const Vector3 pos       = lt0->GetWorldPosition();
      const Vector3 other_pos = rt0->GetWorldPosition();

      Vector3 lhs_normal;
      float   lhs_pen;

      if (!cl->GetPenetration(*cl_other, lhs_normal, lhs_pen)) { return; }

      // Gets the vector from center to collision point.
      const auto lbnd = cl->GetBounding();
      const auto rbnd = cl_other->GetBounding();

      float distance = 0;
      if (!lbnd.TestRay(rbnd, lhs_normal, distance)) { return; }

      Vector3 collision_point = pos - (lhs_normal * distance);
      Vector3 lhs_weight_pen;
      Vector3 rhs_weight_pen;

      Engine::Physics::EvalImpulse
        (
         pos, other_pos, collision_point, lhs_pen, lhs_normal, cl->GetInverseMass(),
         cl_other->GetInverseMass(), rb->GetAngularMomentum(),
         rb_other->GetAngularMomentum(), rb->GetLinearMomentum(),
         rb_other->GetLinearMomentum(), cl->GetInertiaTensor(),
         cl_other->GetInertiaTensor(), linear_vel, other_linear_vel, angular_vel,
         other_angular_vel, lhs_weight_pen, rhs_weight_pen
        );

      if (!rb->IsFixed())
      {
        lt0->SetWorldPosition(pos + lhs_weight_pen);
        rb->SetLinearMomentum(rb->GetLinearMomentum() + linear_vel);
        rb->SetAngularMomentum(rb->GetAngularMomentum() + angular_vel);
        rb->Synchronize();
      }

      if (!rb_other->IsFixed())
      {
        rt0->SetWorldPosition(pos + rhs_weight_pen);
        rb_other->SetLinearMomentum(rb_other->GetLinearMomentum() + other_linear_vel);
        rb_other->SetAngularMomentum(rb_other->GetAngularMomentum() + other_angular_vel);
        rb_other->Synchronize();
      }
    }
  }

  void ConstraintSolver::ResolveSpeculation(const WeakObject& p_lhs, const WeakObject& p_rhs)
  {
    const auto lhs = p_lhs.lock();
    const auto rhs = p_rhs.lock();

    const auto lcl = lhs->GetComponent<Components::Collider>().lock();
    const auto rcl = rhs->GetComponent<Components::Collider>().lock();

    const auto lrb = lhs->GetComponent<Components::Rigidbody>().lock();

    const auto t0 = lhs->GetComponent<Components::Transform>().lock();
    const auto t1 = lrb->GetT1();

    // Move object back to the previous frame position.
    const auto previous_position = t0->GetWorldPreviousPositionPerFrame();
    t0->SetWorldPosition(previous_position);

    const auto lbnd = lcl->GetBounding();
    const auto rbnd = rcl->GetBounding();

    const auto vel = lrb->GetLinearMomentum();
    Vector3    dir;
    vel.Normalize(dir);

    float distance = 0.f;
    // Ray test sanity check, and re-evaluate the distance.
    if (!lbnd.TestRay(rbnd, dir, distance)) { return; }

    // Move object to the new position.
    const auto new_pos = previous_position + (dir * distance);
    t0->SetWorldPosition(new_pos);
    // Change the future position to preventing the tunneling.
    lrb->Synchronize();
  }
} // namespace Engine::Manager::Physics
