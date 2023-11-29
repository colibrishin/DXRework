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
				const auto speculation_check = GetCollisionDetector().IsSpeculated(
					thisComp->GetOwner().lock()->GetID(), otherComp->GetOwner().lock()->GetID());

				if (speculation_check)
				{
					thisComp->AddSpeculationObject(otherComp->GetOwner().lock()->GetID());
				}

				const auto collision_check = GetCollisionDetector().IsCollided(
					thisComp->GetOwner().lock()->GetID(), otherComp->GetOwner().lock()->GetID());

				const auto collision_frame = GetCollisionDetector().IsCollidedInFrame(
					thisComp->GetOwner().lock()->GetID(), otherComp->GetOwner().lock()->GetID());

				if (collision_frame)
				{
					thisComp->GetOwner().lock()->OnCollisionEnter(*otherComp);
					thisComp->AddCollidedObject(otherComp->GetOwner().lock()->GetID());
				}
				else if (collision_check)
				{
					thisComp->GetOwner().lock()->OnCollisionContinue(*otherComp);
				}
				else if (!collision_check && !collision_frame)
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

	void Object::OnCreate()
	{
		if (const auto scene = GetScene().lock())
		{
			scene->AddGameObject(GetSharedPtr<Object>(), GetLayer());
		}

		for (const auto& comp : m_priority_sorted_) 		
		{
			if (const auto locked = comp.lock())
			{
				locked->GetSharedPtr<ActorInterface>()->OnCreate();
			}
		}
	}

	void Object::OnDestroy()
	{
		if (const auto scene = GetScene().lock())
		{
			scene->RemoveGameObject(GetID(), GetLayer());
		}

		for (const auto& comp : m_priority_sorted_) 		
		{
			if (const auto locked = comp.lock())
			{
				locked->GetSharedPtr<ActorInterface>()->OnDestroy();
			}
		}
	}

	void Object::OnLayerChanging()
	{
		for (const auto& comp : m_priority_sorted_) 		
		{
			if (const auto locked = comp.lock())
			{
				locked->GetSharedPtr<ActorInterface>()->OnLayerChanging();
			}
		}
	}

	void Object::OnLayerChanged()
	{
		for (const auto& comp : m_priority_sorted_) 		
		{
			if (const auto locked = comp.lock())
			{
				locked->GetSharedPtr<ActorInterface>()->OnLayerChanged();
			}
		}
	}

	void Object::OnSceneChanging()
	{
		for (const auto& comp : m_priority_sorted_) 		
		{
			if (const auto locked = comp.lock())
			{
				locked->GetSharedPtr<ActorInterface>()->OnSceneChanging();
			}
		}
	}

	void Object::OnSceneChanged()
	{
		for (const auto& comp : m_priority_sorted_) 		
		{
			if (const auto locked = comp.lock())
			{
				locked->GetSharedPtr<ActorInterface>()->OnSceneChanged();
			}
		}
	}

	void Object::Initialize()
	{
		Initialize_INTERNAL();
		OnCreate();
	}

	void Object::Render(const float dt)
	{
		if (m_culled_ && !GetProjectionFrustum().CheckRender(GetWeakPtr<Object>()))
		{
			return;
		}

		for (const auto& component : m_priority_sorted_)
		{
			if (const auto locked = component.lock())
			{
				locked->Render(dt);
			}
			else
			{
				m_priority_sorted_.erase(component);
			}
		}

		for (const auto& resource : m_resources_)
		{
			if (const auto locked = resource.lock())
			{
				locked->Render(dt);
			}
			else
			{
				m_resources_.erase(resource);
			}
		}
	}

	void Object::FixedUpdate(const float& dt)
	{
		for (const auto& component : m_priority_sorted_)
		{
			if (const auto locked = component.lock())
			{
				locked->FixedUpdate(dt);
			}
			else
			{
				m_priority_sorted_.erase(component);
			}
		}

		for (const auto& resource : m_resources_)
		{
			if (const auto locked = resource.lock())
			{
				locked->FixedUpdate(dt);
			}
			else 			
			{
				m_resources_.erase(resource);
			}
		}
	}
}
