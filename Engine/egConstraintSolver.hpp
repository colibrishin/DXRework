#pragma once
#include "egCollider.hpp"
#include "egCollision.h"
#include "egElastic.h"

namespace Engine::Manager::Physics
{
	class ConstraintSolver : public Abstract::Singleton<ConstraintSolver>
	{
	public:
		explicit ConstraintSolver(SINGLETON_LOCK_TOKEN) : Singleton() {}
		void Initialize() override;
		void PreUpdate(const float& dt) override;
		
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void FixedUpdate(const float& dt) override;

	private:
		void CheckCollision(Abstract::Object& obj);
		void CheckSpeculation(Abstract::Object& obj);

		void ResolveCollision(Abstract::Object& lhs, Abstract::Object& rhs);
		void ResolveSpeculation(Abstract::Object& lhs, Abstract::Object& rhs);

		std::set<std::pair<uint64_t, uint64_t>> m_collision_resolved_set_;
		std::set<std::pair<uint64_t, uint64_t>> m_speculative_resolved_set_;

	};
}
