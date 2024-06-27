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

    void OnImGui() override;

    void SetCollisionLayer(eLayerType layer, eLayerType mask);
    void UnsetCollisionLayer(eLayerType layer, eLayerType layer2);
    bool IsCollisionLayer(eLayerType layer1, eLayerType layer2);

    bool IsCollided(GlobalEntityID id) const;
    bool IsCollided(GlobalEntityID id1, GlobalEntityID id2) const;
    bool IsCollidedInFrame(GlobalEntityID id1, GlobalEntityID id2) const;

    concurrent_vector<CollisionInfo>& GetCollisionInfo();

  private:
    friend struct SingletonDeleter;
    ~CollisionDetector() override = default;

    void TestCollision(const WeakObjectBase& p_lhs, const WeakObjectBase& p_rhs);
    void TestSpeculation(const WeakObjectBase & p_lhs, const WeakObjectBase & p_rhs, const float dt);

    void DispatchInactiveExit(const WeakObjectBase& lhs);

    std::mutex m_layer_mask_mutex_;
    bool       m_layer_mask_[LAYER_MAX][LAYER_MAX];

    concurrent_vector<CollisionInfo> m_collision_produce_queue_;

    concurrent_map<GlobalEntityID, std::set<GlobalEntityID>> m_collision_map_;
    concurrent_map<GlobalEntityID, std::set<GlobalEntityID>> m_frame_collision_map_;
  };
} // namespace Engine::Manager


REGISTER_TYPE(Engine::Manager::Application, Engine::Manager::Physics::CollisionDetector)