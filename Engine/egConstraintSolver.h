#pragma once
#include "egCollision.h"
#include "egElastic.h"
#include "egManager.hpp"

namespace Engine::Manager::Physics
{
    class ConstraintSolver : public Abstract::Singleton<ConstraintSolver>
    {
    public:
        explicit ConstraintSolver(SINGLETON_LOCK_TOKEN)
        : Singleton() {}

        void Initialize() override;
        void PreUpdate(const float& dt) override;

        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PostUpdate(const float& dt) override;

    private:
        friend struct SingletonDeleter;
        ~ConstraintSolver() override = default;

        void ResolveCollision(const WeakObject& p_lhs, const WeakObject& p_rhs);
        void ResolveSpeculation(const WeakObject& p_lhs, const WeakObject& p_rhs);

        std::set<std::pair<GlobalEntityID, GlobalEntityID>> m_collision_resolved_set_;
    };
} // namespace Engine::Manager::Physics
