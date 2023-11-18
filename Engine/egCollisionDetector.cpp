#include "pch.hpp"

#include "egCollider.hpp"
#include "egRigidbody.hpp"
#include "egCollisionDetector.hpp"
#include "egSceneManager.hpp"
#include "egCollision.h"
#include "egElastic.h"

namespace Engine::Manager
{
	void CollisionDetector::Initialize()
	{
		for (int i = 0; i < LAYER_MAX; ++i)
		{
			for(int j = 0; j < LAYER_MAX; ++j)
			{
				m_layer_mask_[i].set(j, true);
			}
		}
	}

	void CollisionDetector::ResolveCollision(Abstract::Object* lhs, Abstract::Object* rhs)
	{
		const auto rb = lhs->GetComponent<Engine::Component::Rigidbody>().lock();
		const auto tr = lhs->GetComponent<Engine::Component::Transform>().lock();
		const auto cl = lhs->GetComponent<Engine::Component::Collider>().lock();

		const auto cl_other = rhs->GetComponent<Engine::Component::Collider>().lock();
		const auto rb_other = rhs->GetComponent<Engine::Component::Rigidbody>().lock();
		const auto tr_other = rhs->GetComponent<Engine::Component::Transform>().lock();

		if(rb && cl && rb_other && cl_other)
		{
			if (rb->IsFixed())
			{
				return;
			}

			if (m_collision_resolved_set_.contains({ lhs->GetID(), rhs->GetID() }) ||
				m_collision_resolved_set_.contains({ rhs->GetID(), lhs->GetID() }))
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

			// @todo: lerp is not applied here.
			tr->SetPosition(pos);
			rb->AddLinearMomentum(linear_vel);
			rb->AddAngularMomentum(angular_vel);

			if (!rb_other->IsFixed())
			{
				tr_other->SetPosition(other_pos);
				rb_other->AddLinearMomentum(other_linear_vel);
				rb_other->AddAngularMomentum(other_angular_vel);
			}

			m_collision_resolved_set_.insert({ lhs->GetID(), rhs->GetID() });
		}
	}

	void CollisionDetector::SpeculateCollision(Abstract::Object* lhs, Abstract::Object* rhs)
	{
		const auto rb = lhs->GetComponent<Component::Rigidbody>().lock();
		const auto cl = lhs->GetComponent<Component::Collider>().lock();
		const auto tr = lhs->GetComponent<Component::Transform>().lock();

		const auto rb_other = rhs->GetComponent<Component::Rigidbody>().lock();
		const auto cl_other = rhs->GetComponent<Component::Collider>().lock();
		const auto tr_other = rhs->GetComponent<Component::Transform>().lock();

		if (rb && cl && tr && rb_other && cl_other && tr_other)
		{
			if (m_speculative_resolved_set_.contains({ lhs->GetID(), rhs->GetID() }) ||
				m_speculative_resolved_set_.contains({ rhs->GetID(), lhs->GetID() }))
			{
				return;
			}

			Ray ray{};
			ray.position = tr->GetPosition();
			const auto normal = tr_other->GetPosition() - tr->GetPosition();
			normal.Normalize(ray.direction);

			const auto velocity = rb->GetLinearMomentum();
			const auto length = velocity.Length();

			float intersection_distance = 0.0f;
			if (cl->Intersects(ray, length, intersection_distance))
			{
				const Vector3 speculated_pos = tr->GetPreviousPosition() + (ray.direction * intersection_distance);
				tr->SetPosition(speculated_pos);
				cl->SetPosition(speculated_pos);

				m_speculative_resolved_set_.insert({ lhs->GetID(), rhs->GetID() });
			}
		}
	}

	void CollisionDetector::CheckCollision(const std::vector<WeakObject>& lhs, const std::vector<WeakObject>& rhs)
	{
		for (const auto& lhs_obj : lhs)
		{
			for (const auto& rhs_obj : rhs)
			{
				const auto obj = lhs_obj.lock();
				const auto obj_other = rhs_obj.lock();

				if (obj == obj_other)
				{
					continue;
				}

				if (!obj->GetActive() || !obj_other->GetActive())
				{
					continue;
				}

				const auto cl = obj->GetComponent<Component::Collider>().lock();
				const auto cl_other = obj_other->GetComponent<Component::Collider>().lock();

				if (!cl || !cl_other)
				{
					continue;
				}


				if (cl->Intersects(*cl_other))
				{
					if (m_collision_map_[obj->GetID()].contains(obj_other->GetID()))
					{
						obj->DispatchComponentEvent(cl, cl_other);
						obj_other->DispatchComponentEvent(cl_other, cl);
						ResolveCollision(obj.get(), obj_other.get());
						continue;
					}

					m_collision_map_[obj->GetID()].insert(obj_other->GetID());
					m_collision_map_[obj_other->GetID()].insert(obj->GetID());

					obj->DispatchComponentEvent(cl, cl_other);
					obj_other->DispatchComponentEvent(cl_other, cl);
				}
				else
				{
					if (m_collision_map_[obj->GetID()].contains(obj_other->GetID()))
					{
						m_collision_map_[obj->GetID()].erase(obj_other->GetID());
						m_collision_map_[obj_other->GetID()].erase(obj->GetID());

						obj->DispatchComponentEvent(cl, cl_other);
						obj_other->DispatchComponentEvent(cl_other, cl);
					}
				}
			}
		}
	}

	void CollisionDetector::CheckGrounded(const std::vector<WeakObject>& lhs,
		const std::vector<WeakObject>& rhs)
	{
		for (const auto& obj_i : lhs)
		{
			const auto obj_i_locked = obj_i.lock();
			const auto i_rb = obj_i_locked->GetComponent<Component::Rigidbody>().lock();
			const auto obj_i_collider = obj_i_locked->GetComponent<Component::Collider>().lock();

			if (!i_rb || !obj_i_locked->GetActive() || !obj_i_collider || i_rb->IsGrounded() || i_rb->IsFixed())
			{
				continue;
			}

			const auto tr = obj_i_locked->GetComponent<Component::Transform>().lock();

			for (const auto& obj_j : rhs)
			{
				const auto obj_j_locked = obj_j.lock();

				if (obj_i_locked == obj_j_locked)
				{
					continue;
				}

				if (!obj_j_locked->GetActive())
				{
					continue;
				}

				const auto obj_j_collider = obj_j_locked->GetComponent<Component::Collider>().lock();

				if (!obj_j_collider)
				{
					continue;
				}

				if (obj_i_collider->GetPosition().y < obj_j_collider->GetPosition().y)
				{
					continue;
				}

				Component::Collider copy = *obj_i_collider;
				copy.SetPosition(tr->GetPosition() + Vector3::Down * g_epsilon);

				if (copy.Intersects(*obj_j_collider))
				{
					// Ground flag is automatically set to false on the start of the frame.
					i_rb->SetGrounded(true);
				}
			}
		}
	}

	bool CollisionDetector::CheckRaycasting(const std::shared_ptr<Abstract::Object>& lhs,
		const std::shared_ptr<Abstract::Object>& rhs)
	{
		const auto rb = lhs->GetComponent<Component::Rigidbody>().lock();
		const auto cl = lhs->GetComponent<Component::Collider>().lock();
		const auto tr = lhs->GetComponent<Component::Transform>().lock();

		const auto rb_other = rhs->GetComponent<Component::Rigidbody>().lock();
		const auto cl_other = rhs->GetComponent<Component::Collider>().lock();
		const auto tr_other = rhs->GetComponent<Component::Transform>().lock();

		if (rb && cl && tr && rb_other && cl_other && tr_other)
		{
			static Ray ray{};
			ray.position = tr->GetPosition();
			ray.direction = tr_other->GetPosition() - tr->GetPosition();

			const auto velocity = rb->GetLinearMomentum();
			const auto length = velocity.Length();
			float dist;

			return cl_other->Intersects(ray, length, dist);
		}

		return false;
	}

	void CollisionDetector::Update(const float& dt)
	{
		const auto scene = GetSceneManager().GetActiveScene().lock();

		for (int i = 0; i < LAYER_MAX; ++i)
		{
			for (int j = 0; j < LAYER_MAX; ++j)
			{
				if (!m_layer_mask_[i].test(j))
				{
					continue;
				}

				const auto& layer_i = scene->GetGameObjects((eLayerType)i);
				const auto& layer_j = scene->GetGameObjects((eLayerType)j);

				CheckGrounded(layer_i, layer_j);
				CheckCollision(layer_i, layer_j);
			}
		}

		m_collision_resolved_set_.clear();
		m_speculative_resolved_set_.clear();
	}

	void CollisionDetector::PreUpdate(const float& dt)
	{
	}

	void CollisionDetector::PreRender(const float& dt)
	{
	}

	void CollisionDetector::Render(const float& dt)
	{
	}

	void CollisionDetector::FixedUpdate(const float& dt)
	{
	}
}
