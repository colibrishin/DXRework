#pragma once
#include <bitset>
#include <array>

#include "egCommon.hpp"
#include "egManager.hpp"

namespace Engine::Manager
{
	class CollisionManager : public Abstract::Singleton<CollisionManager>
	{
	public:
		explicit CollisionManager(SINGLETON_LOCK_TOKEN) : Singleton() {}
		~CollisionManager() override = default;

		void Initialize() override;
		void CheckCollision(const std::vector<WeakObject>& layer_i, const std::vector<WeakObject>& layer_j);

		void Update() override;
		void PreUpdate() override;
		void PreRender() override;
		void Render() override;
		void FixedUpdate() override;

		bool IsCollided(uint64_t id) const { return !m_collision_map_.at(id).empty(); }
		bool IsCollided(uint64_t id1, uint64_t id2) const { return m_collision_map_.at(id1).contains(id2); }


	private:
		void ResolveCollision(const std::shared_ptr<Abstract::Object>& lhs, const std::shared_ptr<Abstract::Object>& rhs);

		std::array<std::bitset<LAYER_MAX>, LAYER_MAX> m_layer_mask_;
		std::map<uint64_t, std::set<uint64_t>> m_collision_map_;

	};
<<<<<<< refs/remotes/origin/rigidbody-v2

	inline void CollisionManager::Initialize()
	{
		for (int i = 0; i < LAYER_MAX; ++i)
		{
			for(int j = 0; j < LAYER_MAX; ++j)
			{
				m_layer_mask_[i].set(j, true);
			}
		}
	}

	inline void CollisionManager::CheckGravity(const Component::Collider* left, Component::Collider* right)
	{
		Component::Collider copy = *left;

		copy.SetPosition(copy.GetPosition() - Vector3{0.0f, g_epsilon, 0.0f});

		if (copy.Intersects(*right))
		{
			left->GetOwner().lock()->GetComponent<Component::Rigidbody>().lock()->SetFreefalling(false);
		}
		else
		{
			left->GetOwner().lock()->GetComponent<Component::Rigidbody>().lock()->SetFreefalling(true);
		}
	}

	inline void CollisionManager::CheckCollision(const std::vector<WeakObject>& layer_i, const std::vector<WeakObject>& layer_j)
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

					if (i_rb && j_rb && !stage_collision_table[obj_i_locked->GetID()].contains(obj_j_locked->GetID()))
					{
						ApplyReflection(i_rb.get(), j_rb.get());
						CheckGravity(obj_i_collider.get(), obj_j_collider.get());

						stage_collision_table[obj_i_locked->GetID()].insert(obj_j_locked->GetID());
						stage_collision_table[obj_j_locked->GetID()].insert(obj_i_locked->GetID());
					}

					if (m_collision_map_[obj_i_locked->GetID()].contains(obj_j_locked->GetID()))
					{
						continue;
					}

					m_collision_map_[obj_i_locked->GetID()].insert(obj_j_locked->GetID());
					m_collision_map_[obj_j_locked->GetID()].insert(obj_i_locked->GetID());

					obj_i_locked->DispatchComponentEvent(obj_i_collider, obj_j_collider);
					obj_j_locked->DispatchComponentEvent(obj_j_collider, obj_i_collider);
				}
				else
				{
					if (m_collision_map_[obj_i_locked->GetID()].contains(obj_j_locked->GetID()))
					{
						CheckGravity(obj_i_collider.get(), obj_j_collider.get());

						m_collision_map_[obj_i_locked->GetID()].erase(obj_j_locked->GetID());
						m_collision_map_[obj_j_locked->GetID()].erase(obj_i_locked->GetID());

						obj_i_locked->DispatchComponentEvent(obj_i_collider, obj_j_collider);
						obj_j_locked->DispatchComponentEvent(obj_j_collider, obj_i_collider);
					}
				}
			}
		}
	}

	inline void CollisionManager::Update()
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

	inline void CollisionManager::PreUpdate()
	{
	}

	inline void CollisionManager::PreRender()
	{
	}

	inline void CollisionManager::Render()
	{
	}

	inline void CollisionManager::FixedUpdate()
	{
	}

	inline void CollisionManager::ApplyReflection(Component::Rigidbody* left, Component::Rigidbody* right)
	{
		if (left->GetElastic() && right->GetElastic())
		{
			const auto vel = left->GetVerlet();
			const auto vel_other = right->GetVerlet();

			const auto mass = left->GetMass();
			const auto mass_other = right->GetMass();

			const auto new_vel = ((mass - mass_other) / (mass + mass_other)) * vel;
			const auto new_vel_other = (2 * mass / (mass + mass_other)) * vel;

			left->SetInternalVelocity(new_vel);
			right->SetInternalVelocity(new_vel_other);
		}
		else if (left->GetElastic() ^ right->GetElastic())
		{
			if (!left->GetElastic()) 
			{
				return;
			}

			const auto vel = left->GetVerlet();
			const auto vel_other = right->GetVerlet();

			const auto length = vel.Length();

			if (length <= g_epsilon)
			{
				return;
			}

			const auto mass = left->GetMass();
			const auto mass_other = right->GetMass();

			const auto new_vel = (vel * (mass - mass_other) + 2.0f * mass_other * vel_other) / (mass + mass_other);

			left->SetInternalVelocity(new_vel);
		}
	}
=======
>>>>>>> collisionManager: split dec/impl and rework collision
}
