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

    void LerpManager::PreUpdate(const float& dt)
    {
        if (const auto scene = GetSceneManager().GetActiveScene().lock())
        {
            const auto& rbs = scene->GetCachedComponents<Component::Rigidbody>();

            for (const auto& rb : rbs)
            {
                if (const auto rigidbody = rb.lock())
                {
                    const auto tr = rigidbody->GetOwner()
                                             .lock()
                                             ->GetComponent<Component::Transform>()
                                             .lock();
                    const auto cls =
                            rigidbody->GetOwner().lock()->GetComponents<Component::Collider>();

                    if (tr)
                    {
                        if (!tr->IsTicked())
                        {
                            tr->m_previous_position_ = tr->m_position_;
                        }

                        const auto previous = tr->GetPreviousPosition();
                        const auto current  = tr->GetPosition();
                        const auto lerp     = Vector3::Lerp(previous, current, GetLerpFactor());
                        Vector3CheckNanException(lerp);

                        tr->SetPosition(lerp);
                    }

                    for (const auto& cl : cls)
                    {
                        if (const auto collider = cl.lock())
                        {
                            if (!collider->IsTicked())
                            {
                                collider->m_previous_position_ = collider->m_position_;
                            }

                            const auto previous = collider->GetPreviousPosition();
                            const auto current  = collider->GetPosition();
                            const auto lerp     = Vector3::Lerp(previous, current, GetLerpFactor());
                            Vector3CheckNanException(lerp);

                            collider->SetPosition(lerp);
                        }
                    }
                }
            }
        }

        m_elapsedTime_ += dt;
    }

    void LerpManager::PreRender(const float& dt) {}

    void LerpManager::Render(const float& dt) {}

    void LerpManager::PostRender(const float& dt) {}

    void LerpManager::FixedUpdate(const float& dt)
    {
        Reset();
    }

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
