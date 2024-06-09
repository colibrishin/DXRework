#pragma once
#include "egManager.hpp"

namespace Engine::Manager::Graphics
{
  class GarbageCollector final : public Abstract::Singleton<GarbageCollector>
  {
  public:
    explicit GarbageCollector(SINGLETON_LOCK_TOKEN) {}

    void Initialize() override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostUpdate(const float& dt) override;

    void Track(const ComPtr<ID3D12Resource>& resource);

  private:
    friend struct SingletonDeleter;
    ~GarbageCollector() override = default;

    std::vector<std::pair<UINT64, ComPtr<ID3D12Resource>>> m_track_objects_;
  };
} // namespace Engine::Manager::Graphics
