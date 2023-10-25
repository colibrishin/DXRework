#include "pch.hpp"

#include "egProjectionFrustum.hpp"
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

		if (const auto rb = GetComponent<Engine::Component::Rigidbody>().lock())
		{
			if (const auto rb_other = other.GetOwner().lock()->GetComponent<Engine::Component::Rigidbody>().lock())
			{
				const auto tr = GetComponent<Engine::Component::Transform>().lock();
				const auto tr_other = other.GetOwner().lock()->GetComponent<Engine::Component::Transform>().lock();

				Vector3 dir = tr->GetPosition() - tr_other->GetPosition();
				dir.Normalize();
				dir = XMVector3Orthogonal(dir);

				rb->AddFriction(dir * rb_other->GetFriction());
			}
		}
	}

	void Object::OnCollisionExit(const Engine::Component::Collider& other)
	{
		if (!GetComponent<Engine::Component::Collider>().lock())
		{
			throw std::exception("Object has no collider");
		}

		if (const auto rb = GetComponent<Engine::Component::Rigidbody>().lock())
		{
			if (const auto rb_other = other.GetOwner().lock()->GetComponent<Engine::Component::Rigidbody>().lock())
			{
				const auto tr = GetComponent<Engine::Component::Transform>().lock();
				const auto tr_other = other.GetOwner().lock()->GetComponent<Engine::Component::Transform>().lock();

				Vector3 dir = tr->GetPosition() - tr_other->GetPosition();
				dir.Normalize();
				dir = XMVector3Orthogonal(dir);

				rb->SubtractFriction(dir * rb_other->GetFriction());
			}
		}
	}

	void Object::Render()
	{
		if (m_culled_ && !Engine::Manager::ProjectionFrustum::GetInstance()->CheckRender(GetWeakPtr<Object>()))
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
}
