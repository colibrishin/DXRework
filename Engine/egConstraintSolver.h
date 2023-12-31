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
        void ResolveCollision(const WeakObject & lhs, const WeakObject & rhs);
        void ResolveSpeculation(const WeakObject & lhs, const WeakObject & rhs);

    private:
        std::set<std::pair<GlobalEntityID, GlobalEntityID>> m_collision_resolved_set_;
        std::set<std::pair<GlobalEntityID, GlobalEntityID>> m_speculative_resolved_set_;
    };
} // namespace Engine::Manager::Physics
