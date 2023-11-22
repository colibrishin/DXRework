#include "pch.hpp"

#include "egCollider.hpp"
#include "egRigidbody.hpp"
#include "egCollisionDetector.hpp"
#include "egTransformLerpManager.hpp"
#include "egSceneManager.hpp"
#include "egCollision.h"
#include "egElastic.h"
#include "egManagerHelper.hpp"

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

	void CollisionDetector::CheckCollision(const std::vector<WeakObject>& lhs, const std::vector<WeakObject>& rhs)
	{
		for (const auto& lhs_obj : lhs)
		{
			const auto obj = lhs_obj.lock();
			const auto tr = obj->GetComponent<Component::Transform>().lock();
			const auto rb = obj->GetComponent<Component::Rigidbody>().lock();
			const auto cl = obj->GetComponent<Component::Collider>().lock();

			if (!tr || !cl || !obj->GetActive())
			{
				continue;
			}

			BoundingSphere speculation_area{};

			const auto velocity = rb ? rb->GetLinearMomentum().Length() : g_epsilon;
			const auto speculation_radius = velocity;

			speculation_area.Center = tr->GetPreviousPosition();
			speculation_area.Radius = speculation_radius;

			for (const auto& rhs_obj : rhs)
			{
				const auto obj_other = rhs_obj.lock();
				const auto cl_other = obj_other->GetComponent<Component::Collider>().lock();

				if (obj == obj_other)
				{
					continue;
				}

				if (!obj_other->GetActive())
				{
					continue;
				}

				if (!cl_other)
				{
					continue;
				}

				bool speculation = false;

				if (cl_other->GetType() == BOUNDING_TYPE_BOX)
				{
					speculation = speculation_area.Intersects(cl_other->As<BoundingOrientedBox>());
				}
				else if (cl_other->GetType() == BOUNDING_TYPE_SPHERE)
				{
					speculation = speculation_area.Intersects(cl_other->As<BoundingSphere>());
				}

				if (speculation || CheckRaycasting(obj, obj_other))
				{
					if (!m_speculation_map_[obj->GetID()].contains(obj_other->GetID()))
					{
						m_speculation_map_[obj->GetID()].insert(obj_other->GetID());

						m_frame_collision_map_[obj->GetID()].insert(obj_other->GetID());

						obj->DispatchComponentEvent(cl, cl_other);
					}
				}

				if (cl->Intersects(*cl_other))
				{
					if (m_collision_map_[obj->GetID()].contains(obj_other->GetID()))
					{
						m_collision_map_[obj->GetID()].insert(obj_other->GetID());

						obj->DispatchComponentEvent(cl, cl_other);
						continue;
					}

					m_frame_collision_map_[obj->GetID()].insert(obj_other->GetID());

					obj->DispatchComponentEvent(cl, cl_other);
				}
				else
				{
					if (m_collision_map_[obj->GetID()].contains(obj_other->GetID()))
					{
						m_collision_map_[obj->GetID()].erase(obj_other->GetID());

						obj->DispatchComponentEvent(cl, cl_other);
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
			ray.position = tr->GetPreviousPosition();
			const auto velocity = rb->GetLinearMomentum();
			velocity.Normalize(ray.direction);

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

				CheckCollision(layer_i, layer_j);
			}
		}
	}

	void CollisionDetector::PreUpdate(const float& dt)
	{
		m_collision_map_.merge(m_frame_collision_map_);
		
		m_frame_collision_map_.clear();
		m_speculation_map_.clear();

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
			}
		}
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

	void CollisionDetector::GetCollidedObjects(const Ray& ray, const float distance, std::set<WeakObject, WeakObjComparer>& out)
	{
		const auto scene = GetSceneManager().GetActiveScene().lock();

		if (!scene)
		{
			GetDebugger().Log(L"CollisionDetector: Scene has not loaded.");
			out = {};
		}

		for (int i = LAYER_NONE; i < LAYER_MAX; ++i)
		{
			const auto layer = scene->GetGameObjects((eLayerType)i);

			for (const auto& obj : layer)
			{
				const auto obj_locked = obj.lock();
				const auto cl = obj_locked->GetComponent<Component::Collider>().lock();

				if (!cl)
				{
					continue;
				}

				float intersection;

				if (cl->Intersects(ray, distance, intersection))
				{
					out.insert(obj);
				}
			}
		}
	}
}
