#include "pch.h"
#include "egComponent.h"

#include "egImGuiHeler.hpp"
#include "egSceneManager.hpp"

SERIALIZE_IMPL
(
 Engine::Abstract::Component,
 _ARTAG(_BSTSUPER(Entity))
 _ARTAG(m_local_id_)
 _ARTAG(m_type_)
)

namespace Engine::Abstract
{
	WeakObjectBase Component::GetOwner() const
	{
		return m_owner_;
	}

	eComponentType Component::GetComponentType() const
	{
		return m_type_;
	}

	LocalComponentID Component::GetLocalID() const
	{
		return m_local_id_;
	}

	bool Component::IsTicked() const
	{
		return m_b_ticked_;
	}

	void Component::SetActive(bool active)
	{
		m_b_active_ = active;
	}

	void Component::Initialize()
	{
		Entity::Initialize();
		SetName(GetPrettyTypeName());
	}

	void Component::PostUpdate(const float& dt)
	{
		m_b_ticked_ = true;
	}

	void Component::OnDeserialized()
	{
		Entity::OnDeserialized();
	}

	bool Component::GetActive() const
	{
		return m_b_active_;
	}

	void Component::OnImGui()
	{
		Entity::OnImGui();
		ImGui::Indent(2);
		lldDisabled("Local ID", m_local_id_);
		CheckboxAligned("Active", m_b_active_);
		ImGui::Unindent(2);
	}

	StrongComponent Component::Clone() const
	{
		const auto& cloned = cloneImpl();
		return cloned;
	}

	Component::Component(eComponentType type, const WeakObjectBase& owner)
		: m_local_id_(g_invalid_id),
		  m_type_(type),
		  m_owner_(owner),
		  m_b_ticked_(false),
		  m_b_active_(true) {}

	void Component::SetOwner(const WeakObjectBase& owner)
	{
		if (const auto obj = owner.lock())
		{
			m_owner_ = owner;
		}
	}
} // namespace Engine::Abstract
