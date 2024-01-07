#pragma once
#include "egManager.hpp"

namespace Engine::Components
{
    class Rigidbody;
}

namespace Engine::Manager::Physics
{
    class PhysicsManager : public Abstract::Singleton<PhysicsManager>
    {
    public:
        explicit PhysicsManager(SINGLETON_LOCK_TOKEN)
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
        ~PhysicsManager() override = default;

        static void UpdateGravity(Components::Rigidbody* rb);
        static void EpsilonGuard(Vector3& linear_momentum);
        static void UpdateObject(Components::Rigidbody* rb, const float& dt);
    };
} // namespace Engine::Manager::Physics
