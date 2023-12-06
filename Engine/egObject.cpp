#include "pch.hpp"

#include "egManagerHelper.hpp"
#include "egObject.hpp"
#include "egCollider.hpp"
#include "egCollision.h"
#include "egRigidbody.hpp"

SERIALIZER_ACCESS_IMPL(
	Engine::Abstract::Object,
	_ARTAG(_BSTSUPER(Actor))
	_ARTAG(m_components_)
	_ARTAG(m_resource_names_))

namespace Engine::Abstract
{
	template void Object::DispatchComponentEvent(Engine::Component::Collider& lhs, Engine::Component::Collider& rhs);

	template <typename T>
	void Object::DispatchComponentEvent(T& lhs, T& rhs)
	{
		if constexpr (std::is_base_of_v<Component, T>)
		{
			if constexpr (std::is_same_v<Engine::Component::Collider, T>)
			{
				const auto speculation_check = GetCollisionDetector().IsSpeculated(
					lhs.GetOwner().lock()->GetID(), rhs.GetOwner().lock()->GetID());

				if (speculation_check)
				{
					lhs.AddSpeculationObject(rhs.GetOwner().lock()->GetID());
				}

				const auto collision_check = GetCollisionDetector().IsCollided(
					lhs.GetOwner().lock()->GetID(), rhs.GetOwner().lock()->GetID());

				const auto collision_frame = GetCollisionDetector().IsCollidedInFrame(
					lhs.GetOwner().lock()->GetID(), rhs.GetOwner().lock()->GetID());

				if (collision_frame)
				{
					lhs.GetOwner().lock()->OnCollisionEnter(rhs);
					lhs.AddCollidedObject(rhs.GetOwner().lock()->GetID());
				}
				else if (collision_check)
				{
					lhs.GetOwner().lock()->OnCollisionContinue(rhs);
				}
				else if (!collision_check && !collision_frame)
				{
					lhs.GetOwner().lock()->OnCollisionExit(rhs);
					lhs.RemoveCollidedObject(rhs.GetOwner().lock()->GetID());
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
		for (const auto& component : m_priority_sorted_)
		{
			if (const auto locked = component.lock())
			{
				if (!locked->GetActive())
				{
					continue;
				}

				locked->Render(dt);
			}
			else
			{
				m_priority_sorted_.erase(component);
			}
		}

		if (m_culled_ && !GetProjectionFrustum().CheckRender(GetWeakPtr<Object>()))
		{
			return;
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
				if (!locked->GetActive())
				{
					continue;
				}

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

	void Object::OnDeserialized()
	{
		Actor::OnDeserialized();

		for (const auto& val : m_components_ | std::views::values)
		{
			for (const auto& comps : val)
			{
				comps->OnDeserialized();
				comps->SetOwner(GetSharedPtr<Object>());
				m_assigned_component_ids_.insert(comps->GetLocalID());
				m_priority_sorted_.insert(comps);
			}
		}

		for (const auto& name : m_resource_names_)
		{
			// @todo: what if object is not loaded yet?
			const auto resource = GetResourceManager().GetResource(name);

			if (const auto locked = resource.lock())
			{
				locked->OnDeserialized();
				m_resources_.insert(locked);
			}
		}
	}

	void Object::OnImGui()
	{
		const auto id = ToTypeName() + " " + GetName() + " " + std::to_string(GetID());

		if (ImGui::Begin(id.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse))
		{
			ImGui::BulletText("Object");
			Actor::OnImGui();
			ImGui::Indent(2);
			ImGui::Checkbox("Active", &m_active_);
			ImGui::Checkbox("Culling", &m_culled_);

			if (ImGui::TreeNode("Components"))
			{
				for (const auto& pointer : m_components_ | std::views::values)
				{
					for (const auto& comp : pointer)
					{
						if (ImGui::TreeNode(comp->ToTypeName().c_str()))
						{
							comp->OnImGui();
							ImGui::TreePop();
							ImGui::Spacing();
						}
						
					}
				}

				ImGui::TreePop();
				ImGui::Spacing();
			}

			if (ImGui::TreeNode("Resources"))
			{
				for (const auto& resource : m_resources_)
				{
					if (const auto locked = resource.lock())
					{
						locked->OnImGui();
					}
				}

				ImGui::TreePop();
				ImGui::Spacing();
			}

			ImGui::Unindent(2);
			ImGui::End();
		}
	}

	void Object::PreUpdate(const float& dt)
	{
		for (const auto& component : m_priority_sorted_)
		{
			if (const auto locked = component.lock())
			{
				if (!locked->GetActive())
				{
					continue;
				}

				locked->PreUpdate(dt);
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
				locked->PreUpdate(dt);
			}
			else
			{
				m_resources_.erase(resource);
			}
		}
	}

	void Object::PreRender(const float dt)
	{
		for (const auto& component : m_priority_sorted_)
		{
			if (const auto locked = component.lock())
			{
				if (!locked->GetActive())
				{
					continue;
				}

				locked->PreRender(dt);
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
				locked->PreRender(dt);
			}
			else
			{
				m_resources_.erase(resource);
			}
		}
	}

	void Object::Update(const float& dt)
	{
		for (const auto& component : m_priority_sorted_)
		{
			if (const auto locked = component.lock())
			{
				if (!locked->GetActive())
				{
					continue;
				}

				locked->Update(dt);
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
				locked->Update(dt);
			}
			else
			{
				m_resources_.erase(resource);
			}
		}
	}
}
