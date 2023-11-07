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
}
