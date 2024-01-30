#include "pch.h"
#include "egDeltaTimeDeviation.h"

namespace Engine::Manager::Physics
{
  void DeltaTimeDeviation::PreUpdate(const float& dt)
  {
    m_samples_.push_back(dt);

    if (m_samples_.size() > g_stabilizer_samples)
    {
      m_samples_.pop_front();
    }

    const auto sum = std::accumulate(m_samples_.begin(), m_samples_.end(), 0.0f);
    const auto avg = sum / static_cast<float>(m_samples_.size());
    float variance = 0.0f;

    for (const auto& sample : m_samples_)
    {
      variance += (sample - avg) * (sample - avg);
    }

    const float std_dev = std::sqrt(variance / static_cast<float>(m_samples_.size()));

    /*
    if (avg + (std_dev * g_stabilizer_dev_sigma_threshold) < dt)
    {
      m_is_stable_ = false;
      GetDebugger().Log
        (std::format("Unstable Deltatime!, stddev: {}, dt: {}", std::to_string(std_dev), std::to_string(dt)));
    }
    else
    {
      m_is_stable_ = true;
    } */
  }

  void DeltaTimeDeviation::Update(const float& dt) {}

  void DeltaTimeDeviation::PostUpdate(const float& dt) {}

  void DeltaTimeDeviation::FixedUpdate(const float& dt) {}

  void DeltaTimeDeviation::PreRender(const float& dt) {}

  void DeltaTimeDeviation::Render(const float& dt) {}

  void DeltaTimeDeviation::PostRender(const float& dt) {}

  void DeltaTimeDeviation::Initialize() {}

  bool DeltaTimeDeviation::Stable() const { return m_is_stable_; }
}
