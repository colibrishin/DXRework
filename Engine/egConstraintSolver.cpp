#include "pch.hpp"
#include "egConstraintSolver.hpp"
#include "egSceneManager.hpp"
#include "egCollisionDetector.hpp"

namespace Engine::Manager::Physics
{
	void ConstraintSolver::Initialize()
	{
	}

	void ConstraintSolver::PreUpdate(const float& dt)
	{
	}

	void ConstraintSolver::Update(const float& dt)
	{
		const auto scene = GetSceneManager().GetActiveScene().lock();

		for (int i = 0; i < LAYER_MAX; ++i)
		{
			const auto& objs = scene->GetGameObjects((eLayerType)i);

			for (auto& object : objs)
			{
				const auto obj = object.lock();

				if (!obj)
				{
					continue;
				}
				
				CheckSpeculation(*obj);
				CheckCollision(*obj);
			}
		}

		m_collision_resolved_set_.clear();
		m_speculative_resolved_set_.clear();
	}

	void ConstraintSolver::PreRender(const float& dt)
	{
	}

	void ConstraintSolver::Render(const float& dt)
	{
	}

	void ConstraintSolver::FixedUpdate(const float& dt)
	{
	}

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

	void ConstraintSolver::ResolveCollision(Abstract::Object& lhs, Abstract::Object& rhs)
	{
		const auto rb = lhs.GetComponent<Engine::Component::Rigidbody>().lock();
		const auto tr = lhs.GetComponent<Engine::Component::Transform>().lock();
		const auto cl = lhs.GetComponent<Engine::Component::Collider>().lock();

		const auto cl_other = rhs.GetComponent<Engine::Component::Collider>().lock();
		const auto rb_other = rhs.GetComponent<Engine::Component::Rigidbody>().lock();
		const auto tr_other = rhs.GetComponent<Engine::Component::Transform>().lock();

		if (rb && cl && rb_other && cl_other)
		{
			if (rb->IsFixed())
			{
				return;
			}

			if (m_collision_resolved_set_.contains({ lhs.GetID(), rhs.GetID() }) ||
				m_collision_resolved_set_.contains({ rhs.GetID(), lhs.GetID() }))
			{
				return;
			}

			Vector3 linear_vel;
			Vector3 angular_vel;

			Vector3 other_linear_vel;
			Vector3 other_angular_vel;

			Vector3 pos = tr->GetPosition();
			Vector3 other_pos = tr_other->GetPosition();

			const Vector3 delta = (other_pos - pos);
			Vector3 dir;
			delta.Normalize(dir);

			Vector3 normal;
			float penetration;

			cl->GetPenetration(*cl_other, normal, penetration);
			const Vector3 point = cl->GetPosition() + normal * penetration;


			Engine::Physics::EvalImpulse(pos, other_pos, point, penetration, normal, cl->GetInverseMass(),
								cl_other->GetInverseMass(), rb->GetAngularMomentum(),
								rb_other->GetAngularMomentum(), rb->GetLinearMomentum(), rb_other->GetLinearMomentum(),
								cl->GetInertiaTensor(), cl_other->GetInertiaTensor(), linear_vel,
								other_linear_vel, angular_vel, other_angular_vel);


			tr->SetPosition(pos);
			cl->SetPosition(pos);
			rb->AddLinearMomentum(linear_vel);
			rb->AddAngularMomentum(angular_vel);

			if (!rb_other->IsFixed())
			{
				tr_other->SetPosition(other_pos);
				cl_other->SetPosition(other_pos);
				rb_other->AddLinearMomentum(other_linear_vel);
				rb_other->AddAngularMomentum(other_angular_vel);
			}
			
			m_collision_resolved_set_.insert({ lhs.GetID(), rhs.GetID() });
			m_collision_resolved_set_.insert({ rhs.GetID(), lhs.GetID() });
		}
	}

	void ConstraintSolver::ResolveSpeculation(Abstract::Object& lhs, Abstract::Object& rhs)
	{
		const auto rb = lhs.GetComponent<Component::Rigidbody>().lock();
		const auto cl = lhs.GetComponent<Component::Collider>().lock();
		const auto tr = lhs.GetComponent<Component::Transform>().lock();

		const auto rb_other = rhs.GetComponent<Component::Rigidbody>().lock();
		const auto cl_other = rhs.GetComponent<Component::Collider>().lock();
		const auto tr_other = rhs.GetComponent<Component::Transform>().lock();

		if (rb && cl && tr && rb_other && cl_other && tr_other)
		{
			if (m_speculative_resolved_set_.contains({ lhs.GetID(), rhs.GetID() }) ||
				m_speculative_resolved_set_.contains({ rhs.GetID(), lhs.GetID() }))
			{
				return;
			}

			Ray ray{};
			ray.position = tr->GetPreviousPosition();
			const auto velocity = rb->GetLinearMomentum();
			velocity.Normalize(ray.direction);

			const auto length = velocity.Length();
			float intersection_distance = 0.0f;

			// Forward collision check
			if (cl_other->Intersects(ray, length, intersection_distance))
			{
				const Vector3 speculated_pos = tr->GetPosition() - (ray.direction * intersection_distance);
				tr->SetPosition(speculated_pos);
				cl->SetPosition(speculated_pos);

				m_speculative_resolved_set_.insert({ lhs.GetID(), rhs.GetID() });
				m_speculative_resolved_set_.insert({ rhs.GetID(), lhs.GetID() });
			}

			cl->RemoveSpeculationObject(rhs.GetID());
			cl_other->RemoveSpeculationObject(lhs.GetID());
		}
	}
}
