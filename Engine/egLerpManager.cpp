#include "pch.h"
#include "egLerpManager.h"
#include "egManagerHelper.hpp"
#include "egRigidbody.h"
#include "egSceneManager.hpp"
#include "egTransform.h"

namespace Engine::Manager::Physics
{
  LerpManager::LerpManager(SINGLETON_LOCK_TOKEN)
    : Singleton(),
      m_elapsed_time_(0.f) {}

  void LerpManager::Initialize() { m_elapsed_time_ = 0.f; }

  void LerpManager::Update(const float& dt) {}

  void LerpManager::Reset()
  {
    // modff = divide integer part and fractional part
    const float f = std::fmod(m_elapsed_time_, g_fixed_update_interval);
    if (f > 0.f) { m_elapsed_time_ = f; }
    else { m_elapsed_time_ = 0.f; }
  }

  void LerpManager::PreUpdate(const float& dt) {}

  void LerpManager::PreRender(const float& dt) {}

  void LerpManager::Render(const float& dt) {}

  void LerpManager::PostRender(const float& dt) {}

  void LerpManager::FixedUpdate(const float& dt)
  {
    Reset();
  }

  void LerpManager::PostUpdate(const float& dt)
  {
    if (const auto scene = GetSceneManager().GetActiveScene().lock())
    {
      if (g_paused)
      {
        return;
      }

      const auto& rbs = scene->GetCachedComponents<Components::Rigidbody>();

      for (const auto& rb : rbs)
      {
        if (const auto rigidbody = rb.lock())
        {
          if (!rigidbody->GetActive()) { continue; }
          if (rigidbody->GetSharedPtr<Components::Rigidbody>()->IsFixed()) { continue; }
          if (!rigidbody->GetSharedPtr<Components::Rigidbody>()->GetLerp()) { continue; }

          const auto t0 = rigidbody->GetOwner().lock()->GetComponent<Components::Transform>().lock();
          const auto t1 = rigidbody->GetSharedPtr<Components::Rigidbody>()->GetT1();

          if (t0 && t1)
          {
            const auto t0pos = t0->GetLocalPosition();
            const auto t1pos  = t1->GetLocalPosition();
            const auto f = GetLerpFactor();
            const auto lerp    = Vector3::Lerp(t0pos, t1pos, f);
            Vector3CheckNanException(lerp);

            const auto t0rot = t0->GetLocalRotation();
            const auto t1rot = t1->GetLocalRotation();
            const auto slerp = Quaternion::Slerp(t0rot, t1rot, f);

            t0->SetLocalPosition(lerp);
            t0->SetLocalRotation(slerp);
          }
        }
      }
    }

    m_elapsed_time_ += dt;
  }

  float LerpManager::GetLerpFactor() const
  {
    const auto f = m_elapsed_time_ / g_fixed_update_interval;
    if (!isfinite(f)) { return g_epsilon_squared; }
    else { return f; }
  }
} // namespace Engine::Manager::Physics
