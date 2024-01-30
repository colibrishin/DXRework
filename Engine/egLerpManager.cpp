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

  void LerpManager::Update(const float& dt)
  {
    if (const auto scene = GetSceneManager().GetActiveScene().lock())
    {
      const auto& rbs = scene->GetCachedComponents<Components::Rigidbody>();

      for (const auto& rb : rbs)
      {
        if (const auto rigidbody = rb.lock())
        {
          const auto tr = rigidbody->GetOwner()
                                   .lock()
                                   ->GetComponent<Components::Transform>()
                                   .lock();

          if (tr)
          {
            if (!tr->IsTicked())
            {
              tr->m_world_previous_position_ = tr->GetWorldPosition();
              tr->m_previous_position_       = tr->GetLocalPosition();
            }

            const auto previous = tr->GetLocalPreviousPosition();
            const auto current  = tr->GetLocalPosition();
            const auto lerp     = Vector3::Lerp(previous, current, GetLerpFactor());
            Vector3CheckNanException(lerp);

            tr->SetLocalPosition(lerp);
          }
        }
      }
    }

    m_elapsed_time_ += dt;
  }

  void LerpManager::Reset() { m_elapsed_time_ = g_epsilon_squared; }

  void LerpManager::PreUpdate(const float& dt) {}

  void LerpManager::PreRender(const float& dt) {}

  void LerpManager::Render(const float& dt) {}

  void LerpManager::PostRender(const float& dt) {}

  void LerpManager::FixedUpdate(const float& dt)
  {
    Reset();
  }

  void LerpManager::PostUpdate(const float& dt) {}

  float LerpManager::GetLerpFactor() const
  {
    return m_elapsed_time_ / g_fixed_update_interval;
  }
} // namespace Engine::Manager::Physics
