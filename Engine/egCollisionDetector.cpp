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
				const auto cls_other = obj_other->GetComponents<Component::Collider>();

				for (const auto& cl_next : cls_other)
				{
					const auto cl_other = cl_next.lock();

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

				const auto obj_j_colliders = obj_j_locked->GetComponents<Component::Collider>();

				for (const auto& obj_j_cl : obj_j_colliders)
				{
					const auto obj_j_collider = obj_j_cl.lock();

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
	}

	bool CollisionDetector::CheckRaycasting(const std::shared_ptr<Abstract::Object>& lhs,
		const std::shared_ptr<Abstract::Object>& rhs)
	{
		const auto rb = lhs->GetComponent<Component::Rigidbody>().lock();
		const auto cl = lhs->GetComponent<Component::Collider>().lock();
		const auto tr = lhs->GetComponent<Component::Transform>().lock();

		const auto rb_other = rhs->GetComponent<Component::Rigidbody>().lock();
		const auto cls_other = rhs->GetComponents<Component::Collider>();
		const auto tr_other = rhs->GetComponent<Component::Transform>().lock();

		if (rb && cl && tr && rb_other && tr_other)
		{
			for (const auto cl_other_unlock : cls_other)
			{
				const auto cl_other = cl_other_unlock.lock();

				if (!cl_other)
				{
					continue;
				}

				static Ray ray{};
				ray.position = tr->GetPreviousPosition();
				const auto velocity = rb->GetLinearMomentum();
				velocity.Normalize(ray.direction);

				const auto length = velocity.Length();
				float dist;

				return cl_other->Intersects(ray, length, dist);
			}
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

		std::mutex out_mutex;

		std::for_each(std::execution::par, scene->serialized_layer_begin(), scene->serialized_layer_end(), [ray, &distance, &out, &out_mutex](const std::pair<const eLayerType, StrongLayer>& layer)
		{
			const auto objects = layer.second->GetGameObjects();

			std::for_each(std::execution::par, objects.begin(), objects.end(), [ray, &distance, &out, &out_mutex](const WeakObject& obj)
			{
				const auto obj_locked = obj.lock();
				const auto cls = obj_locked->GetComponents<Component::Collider>();

				for (const auto& collider : cls)
				{
					const auto cl = collider.lock();

					if (!cl)
					{
						continue;
					}

					float intersection;

					if (cl->Intersects(ray, distance, intersection))
					{
						{
							std::lock_guard lock(out_mutex);
							out.insert(obj);
						}
					}
				}
			});
		});
	}

	bool CollisionDetector::Hitscan(const Ray& ray, const float distance, std::set<WeakObject, WeakObjComparer>& out)
	{
		std::set<WeakObject, WeakObjComparer> intermid_out;
		Engine::GetSceneManager().GetActiveScene().lock()->SearchObjects(
			ray.position, ray.direction, intermid_out, static_cast<int>(distance));

		bool hit = false;

		std::mutex out_mutex;

		std::for_each(
			std::execution::par,
			intermid_out.begin(),
			intermid_out.end(),
			[ray, distance, &hit, &out, &out_mutex](const WeakObject& obj)
			{
				if (const auto locked = obj.lock())
				{
					const auto cls = locked->GetComponents<Engine::Component::Collider>();

					std::for_each(
						std::execution::par,
						cls.begin(),
						cls.end(),
						[ray, distance, &hit, &out, &out_mutex, &obj, locked](const std::weak_ptr<Engine::Component::Collider>& cl_o)
						{
							const auto cl = cl_o.lock();

							if (!cl)
							{
								return;
							}

							float ground;

							if (cl->Intersects(ray, distance, ground))
							{
								Engine::GetDebugger().Log(L"Octree Hit! : " + std::to_wstring(locked->GetID()));

								{
									std::lock_guard lock(out_mutex);
									hit |= true;
									out.insert(obj);
								}
							}
						});
				}
			});

		if (hit)
		{
			return true;
		}

		if (out.empty())
		{
			Engine::GetDebugger().Log(L"Octree hits nothing, trying with bruteforce...");

			Engine::GetCollisionDetector().GetCollidedObjects(ray, distance, out);

			for (const auto& obj : out)
			{
				if (const auto locked = obj.lock())
				{
					Engine::GetDebugger().Log(L"Bruteforce Hit! : " + std::to_wstring(locked->GetID()));
				}
			}

			return true;
		}

		return false;
	}
}
