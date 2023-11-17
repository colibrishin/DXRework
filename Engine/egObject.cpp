#include "pch.hpp"

#include "egManagerHelper.hpp"
#include "egObject.hpp"
#include "egCollider.hpp"
#include "egCollision.h"
#include "egRigidbody.hpp"

namespace Engine::Abstract
{
	template void Object::DispatchComponentEvent(const std::shared_ptr<Engine::Component::Collider>& thisComp, const std::shared_ptr<Engine::Component::Collider>& otherComp);

	template <typename T>
	void Object::DispatchComponentEvent(const std::shared_ptr<T>& thisComp, const std::shared_ptr<T>& otherComp)
	{
		if constexpr (std::is_base_of_v<Component, T>)
		{
			if constexpr (std::is_same_v<Engine::Component::Collider, T>)
			{
				const auto collision_check = GetCollisionDetector().IsCollided(
					thisComp->GetOwner().lock()->GetID(), otherComp->GetOwner().lock()->GetID());

				if (collision_check && !thisComp->IsCollidedObject(otherComp->GetOwner().lock()->GetID()))
				{
					thisComp->GetOwner().lock()->OnCollisionEnter(*otherComp);
					thisComp->AddCollidedObject(otherComp->GetOwner().lock()->GetID());
				}
				else if (collision_check && thisComp->IsCollidedObject(otherComp->GetOwner().lock()->GetID()))
				{
					thisComp->GetOwner().lock()->OnCollisionContinue(*otherComp);
				}
				else if (!collision_check)
				{
					thisComp->GetOwner().lock()->OnCollisionExit(*otherComp);
					thisComp->RemoveCollidedObject(otherComp->GetOwner().lock()->GetID());
				}
			}
		}
	}

	void Object::OnCollisionEnter(const Engine::Component::Collider& other)
	{
		if (!GetComponent<Engine::Component::Collider>().lock())
		{
			throw std::exception("Object has no collider");
		}
	}

	void Object::OnCollisionContinue(const Engine::Component::Collider& other)
	{
		if (!GetComponent<Engine::Component::Collider>().lock())
		{
			throw std::exception("Object has no collider");
		}
	}

	void Object::OnCollisionExit(const Engine::Component::Collider& other)
	{
		if (!GetComponent<Engine::Component::Collider>().lock())
		{
			throw std::exception("Object has no collider");
		}
	}

	void Object::Render(const float dt)
	{
		if (m_culled_ && !GetProjectionFrustum().CheckRender(GetWeakPtr<Object>()))
		{
			return;
		}

		for (const auto& component : m_components_ | std::views::values)
		{
			component->Render(dt);
		}

		for (const auto& resource : m_resources_)
		{
			if (const auto locked = resource.lock())
			{
				locked->Render(dt);
			}
		}
	}

	void Object::FixedUpdate(const float& dt)
	{
		for (const auto& component : m_components_ | std::views::values)
		{
			component->FixedUpdate(dt);
		}

		for (const auto& resource : m_resources_)
		{
			if (const auto locked = resource.lock())
			{
				locked->FixedUpdate(dt);
			}
		}
	}
}
