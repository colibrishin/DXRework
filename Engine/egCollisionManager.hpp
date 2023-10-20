#pragma once
#include <bitset>
#include <array>

#include "egCollider.hpp"
#include "egCommon.hpp"
#include "egManager.hpp"
#include "egSceneManager.hpp"

namespace Engine::Manager
{
	class CollisionManager : public Abstract::Manager
	{
	public:
		~CollisionManager() override = default;

		void Initialize() override;
		void CheckCollision(const std::vector<WeakObject>& layer_i, const std::vector<WeakObject>& layer_j);

		void Update() override;
		void PreUpdate() override;
		void PreRender() override;
		void Render() override;

		static CollisionManager* GetInstance()
		{
			if (!s_instance_)
			{
				s_instance_ = std::unique_ptr<CollisionManager>(new CollisionManager);
				s_instance_->Initialize();
			}

			return s_instance_.get();
		}

	private:
		CollisionManager() = default;

		std::array<std::bitset<LAYER_MAX>, LAYER_MAX> m_layer_mask_;
		std::map<uint64_t, std::set<uint64_t>> m_collision_map_;

		inline static std::unique_ptr<CollisionManager> s_instance_ = nullptr;
	};

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

	inline void CollisionManager::CheckCollision(const std::vector<WeakObject>& layer_i, const std::vector<WeakObject>& layer_j)
	{
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
					if (m_collision_map_[obj_i_locked->GetID()].contains(obj_j_locked->GetID()))
					{
						continue;
					}

					m_collision_map_[obj_i_locked->GetID()].insert(obj_j_locked->GetID());
					m_collision_map_[obj_j_locked->GetID()].insert(obj_i_locked->GetID());

					obj_i_collider->SetCollided(true);
					obj_j_collider->SetCollided(true);
					obj_i_locked->DispatchComponentEvent(obj_i_collider, obj_j_collider);
					obj_j_locked->DispatchComponentEvent(obj_j_collider, obj_i_collider);
				}
				else
				{
					if (m_collision_map_[obj_i_locked->GetID()].contains(obj_j_locked->GetID()))
					{
						m_collision_map_[obj_i_locked->GetID()].erase(obj_j_locked->GetID());
						m_collision_map_[obj_j_locked->GetID()].erase(obj_i_locked->GetID());

						obj_i_collider->SetCollided(false);
						obj_j_collider->SetCollided(false);
						obj_i_locked->DispatchComponentEvent(obj_i_collider, obj_j_collider);
						obj_j_locked->DispatchComponentEvent(obj_j_collider, obj_i_collider);
					}
				}
			}
		}
	}

	inline void CollisionManager::Update()
	{
		const auto scene = SceneManager::GetInstance()->GetActiveScene().lock();

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
}
