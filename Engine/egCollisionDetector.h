#pragma once
#include <array>
#include <bitset>

#include "egCommon.hpp"
#include "egManager.hpp"

namespace Engine::Manager
{
    class CollisionDetector : public Abstract::Singleton<CollisionDetector>
    {
    public:
        explicit CollisionDetector(SINGLETON_LOCK_TOKEN)
        : Singleton() {}

        ~CollisionDetector() override = default;

        void Initialize() override;
        void Update(const float& dt) override;
        void PreUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void FixedUpdate(const float& dt) override;

        void SetCollisionLayer(eLayerType layer, eLayerType mask);

        static void GetCollidedObjects(
            const Ray&                                            ray, float distance,
            std::set<WeakObject, WeakComparer<Abstract::Object>>& out);
        static bool Hitscan(
            const Ray&                                            ray, float distance,
            std::set<WeakObject, WeakComparer<Abstract::Object>>& out);

        bool IsCollided(EntityID id) const;
        bool IsCollided(EntityID id1, EntityID id2) const;
        bool IsCollidedInFrame(EntityID id1, EntityID id2) const;

        bool IsSpeculated(EntityID id1, EntityID id2) const;

    private:
        void CheckCollision(Component::Collider& lhs, Component::Collider& rhs);
        void CheckGrounded(const Component::Collider& lhs, Component::Collider& rhs);
        bool CheckRaycasting(
            const Component::Collider& lhs,
            const Component::Collider& rhs);

    private:
        std::array<std::bitset<LAYER_MAX>, LAYER_MAX> m_layer_mask_;
        std::map<EntityID, std::set<EntityID>>        m_collision_map_;
        std::map<EntityID, std::set<EntityID>>        m_frame_collision_map_;
        std::map<EntityID, std::set<EntityID>>        m_speculation_map_;
    };
} // namespace Engine::Manager
