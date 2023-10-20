#include "pch.hpp"

#include "egObject.hpp"
#include "egCollider.hpp"

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
				if (thisComp->HasCollisionStarted())
				{
					thisComp->GetOwner().lock()->OnCollisionEnter(*otherComp);
				}
				else if (thisComp->HasCollisionEnd())				
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
}
