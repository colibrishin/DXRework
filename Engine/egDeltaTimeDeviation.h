#pragma once
#include "egManager.hpp"

namespace Engine::Manager::Physics
{
  class DeltaTimeDeviation : public Abstract::Singleton<DeltaTimeDeviation>
  {
  public:
    constexpr static size_t g_stabilizer_samples = 100;
    constexpr static size_t g_stabilizer_dev_sigma_threshold = 2;

    explicit DeltaTimeDeviation(SINGLETON_LOCK_TOKEN)
      : m_samples_({}),
        m_is_stable_(true) {}

    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void Initialize() override;

    bool Stable() const;

  private:
    friend struct SingletonDeleter;
    ~DeltaTimeDeviation() override = default;

    std::deque<float> m_samples_;
    bool m_is_stable_;

  };
}
