#include "pch.hpp"

#include "egManagerHelper.hpp"
#include "egObject.hpp"
#include "egCollider.hpp"
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
				const auto collision_check = GetCollisionManager().IsCollided(
					thisComp->GetOwner().lock()->GetID(), otherComp->GetOwner().lock()->GetID());

				if (collision_check)
				{
					thisComp->GetOwner().lock()->OnCollisionEnter(*otherComp);
				}
				else if (!collision_check)
				{
					thisComp->GetOwner().lock()->OnCollisionExit(*otherComp);
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

	void Object::OnCollisionExit(const Engine::Component::Collider& other)
	{
		if (!GetComponent<Engine::Component::Collider>().lock())
		{
			throw std::exception("Object has no collider");
		}
	}

	void Object::Render()
	{
		if (m_culled_ && !GetProjectionFrustum().CheckRender(GetWeakPtr<Object>()))
		{
			return;
		}

		for (const auto& component : m_components_ | std::views::values)
		{
			component->Render();
		}

		for (const auto& resource : m_resources_)
		{
			if (const auto locked = resource.lock())
			{
				locked->Render();
			}
		}
	}

	void Object::FixedUpdate()
	{
		for (const auto& component : m_components_ | std::views::values)
		{
			component->FixedUpdate();
		}

		for (const auto& resource : m_resources_)
		{
			if (const auto locked = resource.lock())
			{
				locked->FixedUpdate();
			}
		}
	}
}
