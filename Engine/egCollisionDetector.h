#pragma once
#include <array>
#include <bitset>

#include "egCommon.hpp"
#include "egManager.hpp"

namespace Engine::Manager::Physics
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
        void PostUpdate(const float& dt) override;

        void SetCollisionLayer(eLayerType layer, eLayerType mask);

        static void GetCollidedObjects(
            const Ray&                                            ray, float distance,
            std::set<WeakObject, WeakComparer<Abstract::Object>>& out);
        static bool Hitscan(
            const Ray&                                            ray, float distance,
            std::set<WeakObject, WeakComparer<Abstract::Object>>& out);

        bool IsCollided(GlobalEntityID id) const;
        bool IsCollided(GlobalEntityID id1, GlobalEntityID id2) const;
        bool IsCollidedInFrame(GlobalEntityID id1, GlobalEntityID id2) const;
        bool IsSpeculated(GlobalEntityID id1, GlobalEntityID id2) const;
        concurrent_vector<CollisionInfo>& GetCollisionInfo();

    private:
        void CheckCollision(StrongBaseCollider& lhs, StrongBaseCollider& rhs);
        void CheckCollisionChunk(StrongBaseCollider & lhs, std::vector<StrongBaseCollider> & rhs);
        void CheckGrounded(const StrongBaseCollider & lhs, const StrongBaseCollider & rhs);
        bool CheckRaycasting(const StrongBaseCollider & lhs, const StrongBaseCollider & rhs);

    private:
        std::mutex                                    m_layer_mask_mutex_;
        std::array<std::bitset<LAYER_MAX>, LAYER_MAX> m_layer_mask_;

        concurrent_vector<CollisionInfo> m_collision_queue_;
        concurrent_map<GlobalEntityID, std::set<GlobalEntityID>>  m_collision_map_;
        concurrent_map<GlobalEntityID, std::set<GlobalEntityID>>  m_frame_collision_map_;
        concurrent_map<GlobalEntityID, std::set<GlobalEntityID>>  m_speculation_map_;
    };
} // namespace Engine::Manager
