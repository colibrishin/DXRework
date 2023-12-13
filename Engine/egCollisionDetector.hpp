#pragma once
#include <bitset>
#include <array>

#include "egCommon.hpp"
#include "egManager.hpp"

namespace Engine::Manager
{
	class CollisionDetector : public Abstract::Singleton<CollisionDetector>
	{
	public:
		explicit CollisionDetector(SINGLETON_LOCK_TOKEN) : Singleton() {}
		~CollisionDetector() override = default;

		void Initialize() override;
		void Update(const float& dt) override;
		void PreUpdate(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		void SetCollisionLayer(const eLayerType layer, const eLayerType mask);

		static void GetCollidedObjects(const Ray& ray, const float distance, std::set<WeakObject, WeakComparer<Abstract::Object>>& out);
		static bool Hitscan(const Ray& ray, const float distance, std::set<WeakObject, WeakComparer<Abstract::Object>>& out);

		bool IsCollided(EntityID id) const
		{
			if (!m_collision_map_.contains(id))
			{
				return false;
			}

			return !m_collision_map_.at(id).empty();
		}
		bool IsCollided(EntityID id1, EntityID id2) const
		{
			if (!m_collision_map_.contains(id1))
			{
				return false;
			}

			return m_collision_map_.at(id1).contains(id2);
		}
		bool IsSpeculated(EntityID id1, EntityID id2) const
		{
			if (!m_speculation_map_.contains(id1))
			{
				return false;
			}

			return m_speculation_map_.at(id1).contains(id2);
		}
		bool IsCollidedInFrame(EntityID id1, EntityID id2) const
		{
			if (!m_frame_collision_map_.contains(id1))
			{
				return false;
			}

			return m_frame_collision_map_.at(id1).contains(id2);
		}

	private:
		void CheckCollision(Component::Collider& lhs, Component::Collider& rhs);
		void CheckGrounded(const Component::Collider& lhs, Component::Collider& rhs);
		bool CheckRaycasting(const Component::Collider& lhs,
							const Component::Collider& rhs);

	private:
		std::array<std::bitset<LAYER_MAX>, LAYER_MAX> m_layer_mask_;
		std::map<EntityID, std::set<EntityID>> m_collision_map_;
		std::map<EntityID, std::set<EntityID>> m_frame_collision_map_;
		std::map<EntityID, std::set<EntityID>> m_speculation_map_;

	};
}
