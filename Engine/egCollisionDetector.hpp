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
		void FixedUpdate(const float& dt) override;

		bool IsCollided(uint64_t id) const { return !m_collision_map_.at(id).empty(); }
		bool IsCollided(uint64_t id1, uint64_t id2) const { return m_collision_map_.at(id1).contains(id2); }


	private:
		void CheckCollision(const std::vector<WeakObject>& layer_i, const std::vector<WeakObject>& layer_j);
		void CheckGrounded(const std::vector<WeakObject>& layer_i, const std::vector<WeakObject>& layer_j);
		bool CheckRaycasting(const std::shared_ptr<Abstract::Object>& obj, const std::shared_ptr<Abstract::Object>& obj_other);

		std::array<std::bitset<LAYER_MAX>, LAYER_MAX> m_layer_mask_;
		std::map<uint64_t, std::set<uint64_t>> m_collision_map_;

	};
}
