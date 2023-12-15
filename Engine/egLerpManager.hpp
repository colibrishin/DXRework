#pragma once
#include "egManager.hpp"

namespace Engine::Manager::Physics
{
    class LerpManager : public Abstract::Singleton<LerpManager>
    {
    public:
        LerpManager(SINGLETON_LOCK_TOKEN)
        : Singleton(),
          m_elapsedTime_(g_epsilon) {}

        ~LerpManager() override = default;

        void Initialize() override;
        void Update(const float& dt) override;

        void Reset();
        void PreUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void FixedUpdate(const float& dt) override;

        float GetLerpFactor() const;

    private:
        float m_elapsedTime_;
    };
} // namespace Engine::Manager::Physics
