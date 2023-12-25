#include "pch.h"
#include "egLerpManager.h"
#include "egCollider.hpp"
#include "egManagerHelper.hpp"
#include "egRigidbody.h"
#include "egSceneManager.hpp"
#include "egTransform.h"

namespace Engine::Manager::Physics
{
    LerpManager::LerpManager(SINGLETON_LOCK_TOKEN)
    : Singleton(),
      m_elapsedTime_(g_epsilon) {}

    void LerpManager::Initialize()
    {
        m_elapsedTime_ = g_epsilon;
    }

    void LerpManager::Update(const float& dt) {}

    void LerpManager::Reset()
    {
        m_elapsedTime_ = g_epsilon;
    }

    void LerpManager::PreUpdate(const float& dt) {}

    void LerpManager::PreRender(const float& dt)
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
                    const auto cls =
                            rigidbody->GetOwner().lock()->GetComponents<Components::Collider>();

                    if (tr)
                    {
                        if (!tr->IsTicked())
                        {
                            tr->m_world_previous_position_ = tr->GetWorldPosition();
                            tr->m_previous_position_ = tr->m_position_;
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

        m_elapsedTime_ += dt;
    }

    void LerpManager::Render(const float& dt) {}

    void LerpManager::PostRender(const float& dt) {}

    void LerpManager::FixedUpdate(const float& dt)
    {
        Reset();
    }

    void LerpManager::PostUpdate(const float& dt) {}

    float LerpManager::GetLerpFactor() const
    {
        auto factor = (static_cast<float>(m_elapsedTime_) /
                       static_cast<float>(g_fixed_update_interval));

        if (!std::isfinite(factor))
        {
            factor = g_epsilon;
        }

        factor = std::clamp(factor, g_epsilon, 1.0f);
        return factor;
    }
} // namespace Engine::Manager::Physics
