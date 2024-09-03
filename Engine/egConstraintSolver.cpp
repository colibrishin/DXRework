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
#ifdef PHYSX_ENABLED
#else
		auto& infos = GetCollisionDetector().GetCollisionInfo();

		static tbb::affinity_partitioner ap;

		for (const auto& info : infos)
		{
			if (info.speculative)
			{
				ResolveSpeculation(info.lhs, info.rhs);
			}
			if (info.collision)
			{
				ResolveCollision(info.lhs, info.rhs);
			}
		}

		infos.clear();
		m_collision_resolved_set_.clear();
#endif
	}

	void ConstraintSolver::PostUpdate(const float& dt) {}

	void ConstraintSolver::ResolveCollision(const WeakObjectBase& p_lhs, const WeakObjectBase& p_rhs)
	{
		auto lhs = p_lhs;
		auto rhs = p_rhs;

		auto rb  = lhs.lock()->GetComponent<Components::Rigidbody>().lock();
		auto lt0 = lhs.lock()->GetComponent<Components::Transform>().lock();

		auto rb_other = rhs.lock()->GetComponent<Components::Rigidbody>().lock();
		auto rt0      = rhs.lock()->GetComponent<Components::Transform>().lock();

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

			auto cl       = lhs.lock()->GetComponent<Components::Collider>().lock();
			auto cl_other = rhs.lock()->GetComponent<Components::Collider>().lock();

			if (rb->IsFixed())
			{
				std::swap(rb, rb_other);
				std::swap(lt0, rt0);
				std::swap(lhs, rhs);
				std::swap(cl, cl_other);
			}

			if (!cl || !cl_other)
			{
				return;
			}

			Vector3 llimp, laimp, rlimp, raimp;

			const Vector3 pos       = lt0->GetWorldPosition();
			const Vector3 other_pos = rt0->GetWorldPosition();

			Vector3 lhs_normal;
			float   lhs_pen;

			if (!cl->GetPenetration(*cl_other, lhs_normal, lhs_pen))
			{
				return;
			}
			

			Vector3 collision_point = pos + (lhs_normal * lhs_pen);
			Vector3 lhs_weight_pen;
			Vector3 rhs_weight_pen;

			Engine::Physics::EvalImpulse
					(
					 pos, other_pos, collision_point, lhs_pen, lhs_normal, cl->GetInverseMass(),
					 cl_other->GetInverseMass(), rb->GetT0AngularVelocity(),
					 rb_other->GetT0AngularVelocity(), rb->GetT0LinearVelocity(),
					 rb_other->GetT0LinearVelocity(), cl->GetInertiaTensor(),
					 cl_other->GetInertiaTensor(), llimp, rlimp, laimp,
					 raimp, lhs_weight_pen, rhs_weight_pen
					);

			if (!rb->IsFixed())
			{
				lt0->SetWorldPosition(pos + lhs_weight_pen);
				rb->SetT0LinearVelocity(llimp);
				rb->SetT0AngularVelocity(laimp);
				rb->Synchronize();
			}

			if (!rb_other->IsFixed())
			{
				rt0->SetWorldPosition(other_pos + rhs_weight_pen);
				rb_other->SetT0LinearVelocity(rlimp);
				rb_other->SetT0AngularVelocity(raimp);
				rb_other->Synchronize();
			}
		}
	}

	void ConstraintSolver::ResolveSpeculation(const WeakObjectBase& p_lhs, const WeakObjectBase& p_rhs)
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

		auto lbnd = lcl->GetBounding();
		auto rbnd = rcl->GetBounding();

		const auto vel = lrb->GetT0LinearVelocity();
		Vector3    dir;
		vel.Normalize(dir);

		float distance = 0.f;
		// Ray test sanity check, and re-evaluate the distance.
		if (!lbnd.TestRay(rbnd, dir, distance))
		{
			return;
		}

		// Bisect the distance to find the closest point.
		float f    = 0.5f;
		UINT  iter = 0;

		while (iter < g_speculation_bisection_max_iteration)
		{
			if (lbnd.TestRay(rbnd, dir, distance))
			{
				distance *= f;
				f *= 0.5f;
				lbnd.Translate(-dir * distance);

				if (distance < g_epsilon)
				{
					break;
				}
			}
			else
			{
				distance *= f;
				f *= 1.5f;
				lbnd.Translate(dir * distance);

				if (distance < g_epsilon)
				{
					break;
				}
			}

			++iter;
		}

		// Move object to the new position.
		const auto new_pos = previous_position + (dir * distance);
		t0->SetWorldPosition(new_pos);
		// Change the future position to preventing the tunneling.
		lrb->Synchronize();
	}
} // namespace Engine::Manager::Physics
