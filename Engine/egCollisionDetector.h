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
        void CheckGrounded(const StrongCollider& lhs, const StrongCollider& rhs);
        bool CheckRaycasting(const StrongCollider& lhs, const StrongCollider& rhs);

        bool CheckCollision(const ConcurrentWeakObjVec& rhsl, const StrongObject& lhs, int idx);
        void CheckCollisionImpl(const StrongCollider& lhs, const StrongCollider& rhs);

        __forceinline void ContinuousColliding(
            const StrongCollider& lhs, const StrongCollider& rhs, const StrongObject& lhs_owner,
            const StrongObject&   rhs_owner) const;
        __forceinline void InFrameColliding(
            const StrongCollider& lhs, const StrongCollider& rhs, const StrongObject& lhs_owner,
            const StrongObject&   rhs_owner);
        __forceinline void ExitColliding(
            const StrongCollider& lhs, const StrongCollider& rhs, const StrongObject& lhs_owner,
            const StrongObject&   rhs_owner);

        __forceinline void IncreaseCollisionCounter(
            const StrongCollider& lhs, const StrongCollider& rhs, const StrongObject& lhs_owner,
            const StrongObject&   rhs_owner);
        __forceinline void RemoveCollisionCounter(
            const StrongCollider& lhs, const StrongCollider& rhs, const StrongObject& lhs_owner,
            const StrongObject&   rhs_owner);

    private:
        std::mutex                                    m_layer_mask_mutex_;
        std::array<std::bitset<LAYER_MAX>, LAYER_MAX> m_layer_mask_;

        concurrent_vector<CollisionInfo> m_collision_produce_queue_;

        concurrent_map<GlobalEntityID, std::set<GlobalEntityID>> m_collision_check_map_;
        concurrent_map<GlobalEntityID, std::set<GlobalEntityID>>  m_collision_map_;
        concurrent_map<GlobalEntityID, std::set<GlobalEntityID>>  m_frame_collision_map_;
        concurrent_map<GlobalEntityID, std::set<GlobalEntityID>>  m_speculation_map_;
    };
} // namespace Engine::Manager
