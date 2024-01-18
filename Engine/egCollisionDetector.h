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
    explicit CollisionDetector(SINGLETON_LOCK_TOKEN) {}

    void Initialize() override;
    void Update(const float& dt) override;
    void PreUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostUpdate(const float& dt) override;

    void SetCollisionLayer(eLayerType layer, eLayerType mask);
    bool IsCollisionLayer(eLayerType layer1, eLayerType layer2);

    bool IsCollided(GlobalEntityID id) const;
    bool IsCollided(GlobalEntityID id1, GlobalEntityID id2) const;
    bool IsCollidedInFrame(GlobalEntityID id1, GlobalEntityID id2) const;

    static bool Hitscan(const Vector3& start, float length, const Vector3& dir, std::vector<WeakObject>& hit_objs);

    concurrent_vector<CollisionInfo>& GetCollisionInfo();

  private:
    friend struct SingletonDeleter;
    ~CollisionDetector() override = default;

    void TestCollision(const WeakObject& p_lhs, const WeakObject& p_rhs);
    void TestSpeculation(const WeakObject& p_lhs, const WeakObject& p_rhs);

    void DispatchInactiveExit(const WeakObject& lhs);

    std::mutex                                    m_layer_mask_mutex_;
    std::array<std::bitset<LAYER_MAX>, LAYER_MAX> m_layer_mask_;

    concurrent_vector<CollisionInfo> m_collision_produce_queue_;

    concurrent_map<GlobalEntityID, std::set<GlobalEntityID>> m_collision_map_;
    concurrent_map<GlobalEntityID, std::set<GlobalEntityID>> m_frame_collision_map_;
  };
} // namespace Engine::Manager
