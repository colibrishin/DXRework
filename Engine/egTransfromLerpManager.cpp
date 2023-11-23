#include "pch.hpp"
#include "egSceneManager.hpp"
#include "egCollider.hpp"
#include "egManagerHelper.hpp"
#include "egTransformLerpManager.hpp"

namespace Engine::Manager::Physics
{
	void TransformLerpManager::Initialize()
	{
		m_elapsedTime_ = 0.0f;
	}

	void TransformLerpManager::Update(const float& dt)
	{
		m_elapsedTime_ += dt;

		if (const auto scene = GetSceneManager().GetActiveScene().lock())
		{
			for (int i = 0; i < LAYER_MAX; ++i)
			{
				for (const auto& object : scene->GetGameObjects((eLayerType)i))
				{
					if (const auto obj = object.lock())
					{
						const auto tr = obj->GetComponent<Component::Transform>().lock();

						if (!tr)
						{
							continue;
						}

						const auto previous = tr->GetPreviousPosition();
						const auto current = tr->GetPosition();

						const auto lerping = Vector3::Lerp(previous, current, GetLerpFactor());

						tr->SetPosition(lerping);

						GetSceneManager().GetActiveScene().lock()->UpdatePosition(obj);

						const auto cl = obj->GetComponent<Component::Collider>().lock();

						if (!cl || !cl->GetDirtyFlag())
						{
							continue;
						}

						cl->SetPosition(tr->GetPosition());
					}
				}
			}
		}
	}

	void TransformLerpManager::Reset()
	{
		m_elapsedTime_ = 0.0f;
	}

	void TransformLerpManager::PreUpdate(const float& dt)
	{
	}

	void TransformLerpManager::PreRender(const float& dt)
	{
	}

	void TransformLerpManager::Render(const float& dt)
	{
	}

	void TransformLerpManager::FixedUpdate(const float& dt)
	{
		Reset();
	}

	float TransformLerpManager::GetLerpFactor() const
	{
		auto factor = (m_elapsedTime_ / g_fixed_update_interval);
		factor = std::clamp(factor, 0.0f, 1.0f);
		return factor;
	}
}
