#pragma once
#include <set>

#include "Singleton.hpp"
#include "TypeLibrary/Public/TypeLibrary.h"

POLYMORPHIC_MANAGER_TYPE_MAP(ENGINE_PHYSICSMANAGER_API, Engine::Managers::ConstraintSolver)

namespace Engine::Managers
{
	class ENGINE_PHYSICSMANAGER_API ConstraintSolver : public Abstracts::Singleton<ConstraintSolver>
	{
	public:
		INLINE_COMPILE_TIME_TYPENAME(ConstraintSolver)
		explicit ConstraintSolver(SINGLETON_LOCK_TOKEN)
			: Singleton() {}

		void Initialize() override;
		void PreUpdate(const float dt) override;

		void Update(const float dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostUpdate(const float dt) override;

	private:
		friend struct SingletonDeleter;
		~ConstraintSolver() override = default;

		void ResolveCollision(const Weak<Abstracts::ObjectBase>& p_lhs, const Weak<Abstracts::ObjectBase>& p_rhs);
		void ResolveSpeculation(const Weak<Abstracts::ObjectBase>& p_lhs, const Weak<Abstracts::ObjectBase>& p_rhs);

		std::set<std::pair<GlobalEntityID, GlobalEntityID>> m_collision_resolved_set_;
	};
} // namespace Engine::Managers
