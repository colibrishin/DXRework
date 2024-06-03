#include "pch.h"
#include "egGarbageCollector.h"

namespace Engine::Manager::Graphics
{
  void GarbageCollector::Initialize() {}

  void GarbageCollector::PreUpdate(const float& dt)
  {
    const auto current_fence_value = GetD3Device().GetFenceValue();

    auto it = m_track_objects_.begin();

    while (it != m_track_objects_.end())
    {
      if (it->first < current_fence_value)
      {
        it = m_track_objects_.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }

  void GarbageCollector::PreRender(const float& dt) {}

  void GarbageCollector::Render(const float& dt) {}

  void GarbageCollector::PostRender(const float& dt) {}

  void GarbageCollector::FixedUpdate(const float& dt) {}

  void GarbageCollector::PostUpdate(const float& dt) {}

  void GarbageCollector::Update(const float& dt) {}

  void GarbageCollector::Track(const ComPtr<ID3D12Resource>& resource)
  {
    m_track_objects_.emplace_back(GetD3Device().GetFenceValue(), resource);
  }
}