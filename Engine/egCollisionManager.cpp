#include "pch.hpp"

#include "egCollider.hpp"
#include "egRigidbody.hpp"
#include "egCollisionManager.hpp"
#include "egSceneManager.hpp"

namespace Engine::Manager
{
	void CollisionManager::Initialize()
	{
		for (int i = 0; i < LAYER_MAX; ++i)
		{
			for(int j = 0; j < LAYER_MAX; ++j)
			{
				m_layer_mask_[i].set(j, true);
			}
		}
	}

	void CollisionManager::CheckCollision(const std::vector<WeakObject>& layer_i, const std::vector<WeakObject>& layer_j)
	{
		std::map<uint64_t, std::set<uint64_t>> stage_collision_table;

		for (const auto& obj_i : layer_i)
		{
			for (const auto& obj_j : layer_j)
			{
				const auto obj_i_locked = obj_i.lock();
				const auto obj_j_locked = obj_j.lock();

				if (obj_i_locked == obj_j_locked)
				{
					continue;
				}

				if (!obj_i_locked->GetActive() || !obj_j_locked->GetActive())
				{
					continue;
				}

				const auto obj_i_collider = obj_i_locked->GetComponent<Component::Collider>().lock();
				const auto obj_j_collider = obj_j_locked->GetComponent<Component::Collider>().lock();

				if (!obj_i_collider || !obj_j_collider)
				{
					continue;
				}

				if (obj_i_collider->Intersects(*obj_j_collider))
				{
					const auto i_rb = obj_i_locked->GetComponent<Component::Rigidbody>().lock();
					const auto j_rb = obj_j_locked->GetComponent<Component::Rigidbody>().lock();

					if (m_collision_map_[obj_i_locked->GetID()].contains(obj_j_locked->GetID()))
					{
						obj_i_locked->DispatchComponentEvent(obj_i_collider, obj_j_collider);
						obj_j_locked->DispatchComponentEvent(obj_j_collider, obj_i_collider);
						continue;
					}

					m_collision_map_[obj_i_locked->GetID()].insert(obj_j_locked->GetID());
					m_collision_map_[obj_j_locked->GetID()].insert(obj_i_locked->GetID());

					obj_i_locked->DispatchComponentEvent(obj_i_collider, obj_j_collider);
					obj_j_locked->DispatchComponentEvent(obj_j_collider, obj_i_collider);

					ResolveCollision(obj_i_locked, obj_j_locked);
				}
				else
				{
					if (m_collision_map_[obj_i_locked->GetID()].contains(obj_j_locked->GetID()))
					{
						m_collision_map_[obj_i_locked->GetID()].erase(obj_j_locked->GetID());
						m_collision_map_[obj_j_locked->GetID()].erase(obj_i_locked->GetID());

						obj_i_locked->DispatchComponentEvent(obj_i_collider, obj_j_collider);
						obj_j_locked->DispatchComponentEvent(obj_j_collider, obj_i_collider);
					}
				}
			}
		}
	}

	void CollisionManager::Update()
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

				CheckCollision(layer_i, layer_j);
			}
		}
	}

	void CollisionManager::PreUpdate()
	{
	}

	void CollisionManager::PreRender()
	{
	}

	void CollisionManager::Render()
	{
	}

	void CollisionManager::FixedUpdate()
	{
	}

	void CollisionManager::ResolveCollision(const std::shared_ptr<Abstract::Object>& lhs, const std::shared_ptr<Abstract::Object>& rhs)
	{
		const auto rb = lhs->GetComponent<Engine::Component::Rigidbody>().lock();
		const auto tr = lhs->GetComponent<Engine::Component::Transform>().lock();
		const auto cl = lhs->GetComponent<Engine::Component::Collider>().lock();

		const auto cl_other = rhs->GetComponent<Engine::Component::Collider>().lock();
		const auto rb_other = rhs->GetComponent<Engine::Component::Rigidbody>().lock();
		const auto tr_other = rhs->GetComponent<Engine::Component::Transform>().lock();

		Vector3 linear_vel;
		Vector3 angular_vel;

		Vector3 other_linear_vel;
		Vector3 other_angular_vel;

		Vector3 pos = tr->GetPosition();
		Vector3 other_pos = tr_other->GetPosition();
		const Vector3 delta = (pos - other_pos);

		Vector3 normal;
		float penetration;

		const float distance = Vector3::Distance(pos, other_pos);
		cl->GetPenetration(*cl_other, normal, penetration);
		const Vector3 point = pos + (normal * cl->GetSize());


		Physics::EvalImpulse(pos, other_pos, point, penetration, normal, cl->GetInverseMass(),
							cl_other->GetInverseMass(), rb->GetAngularMomentum(),
							rb_other->GetAngularMomentum(), rb->GetLinearMomentum(), rb_other->GetLinearMomentum(),
							cl->GetInertiaTensor(), cl_other->GetInertiaTensor(), linear_vel,
							other_linear_vel, angular_vel, other_angular_vel);

		if (!rb->IsFixed())
		{
			tr->SetPosition(pos);
		}

		if (!rb_other->IsFixed())
		{
			tr_other->SetPosition(other_pos);
		}

		rb->AddLinearMomentum(linear_vel);
		rb->AddAngularMomentum(angular_vel);

		rb_other->AddLinearMomentum(other_linear_vel);
		rb_other->AddAngularMomentum(other_angular_vel);
	}
}
