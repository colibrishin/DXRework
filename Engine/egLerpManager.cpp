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
      m_elapsed_time_(g_epsilon_squared) {}

  void LerpManager::Initialize() { m_elapsed_time_ = g_epsilon_squared; }

  void LerpManager::Update(const float& dt) {}

  void LerpManager::Reset() { m_elapsed_time_ = g_epsilon_squared; }

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
      const auto& rbs = scene->GetCachedComponents<Components::Rigidbody>();

      for (const auto& rb : rbs)
      {
        if (const auto rigidbody = rb.lock())
        {
          const auto t0 = rigidbody->GetOwner().lock()->GetComponent<Components::Transform>().lock();
          const auto t1 = rigidbody->GetSharedPtr<Components::Rigidbody>()->GetT1();

          if (t0 && t1)
          {
            const auto current = t0->GetLocalPosition();
            const auto future  = t1->GetLocalPosition();
            const auto lerp    = Vector3::Lerp(current, future, GetLerpFactor());
            Vector3CheckNanException(lerp);

            t0->SetLocalPosition(lerp);
          }
        }
      }
    }

    m_elapsed_time_ += dt;
  }

  float LerpManager::GetLerpFactor() const
  {
    return m_elapsed_time_ / g_fixed_update_interval;
  }
} // namespace Engine::Manager::Physics
