#include "pch.hpp"
#include "egSceneManager.hpp"
#include "egCollider.hpp"
#include "egManagerHelper.hpp"
#include "egLerpManager.hpp"

namespace Engine::Manager::Physics
{
	void LerpManager::Initialize()
	{
		m_elapsedTime_ = 0.0f;
	}

	void LerpManager::Update(const float& dt)
	{
		m_elapsedTime_ += dt;

		if (const auto scene = GetSceneManager().GetActiveScene().lock())
		{
			const auto& rbs = scene->GetComponents<Component::Rigidbody>();

			for (const auto& rb : rbs)
			{
				if (const auto rigidbody = rb.lock())
				{
					const auto tr = rigidbody->GetOwner().lock()->GetComponent<Component::Transform>().lock();
					const auto cls = rigidbody->GetOwner().lock()->GetComponents<Component::Collider>();

					if (tr)
					{
						const auto previous = tr->GetPreviousPosition();
						const auto current = tr->GetPosition();
						const auto lerp = Vector3::Lerp(previous, current, GetLerpFactor());

						tr->SetPosition(lerp);
					}

					for (const auto& cl : cls)
					{
						if (const auto collider = cl.lock())
						{
							const auto previous = collider->GetPreviousPosition();
							const auto current = collider->GetPosition();
							const auto lerp = Vector3::Lerp(previous, current, GetLerpFactor());

							collider->SetPosition(lerp);
						}
					}
				}
			}
		}
	}

	void LerpManager::Reset()
	{
		m_elapsedTime_ = 0.0f;
	}

	void LerpManager::PreUpdate(const float& dt)
	{
	}

	void LerpManager::PreRender(const float& dt)
	{
	}

	void LerpManager::Render(const float& dt)
	{
	}

	void LerpManager::FixedUpdate(const float& dt)
	{
		Reset();
	}

	float LerpManager::GetLerpFactor() const
	{
		auto factor = (m_elapsedTime_ / g_fixed_update_interval);
		factor = std::clamp(factor, 0.0f, 1.0f);
		return factor;
	}
}
