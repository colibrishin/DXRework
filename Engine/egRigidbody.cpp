#include "pch.hpp"
#include "egObject.hpp"
#include "egRigidbody.hpp"
#include "egCollider.hpp"
#include "egKinetic.h"
#include "egScene.hpp"
#include "egTransform.hpp"
#include "egCollider.hpp"

SERIALIZER_ACCESS_IMPL(Engine::Component::Rigidbody,
	_ARTAG(_BSTSUPER(Engine::Abstract::Component))
	_ARTAG(m_bGrounded)
	_ARTAG(m_bGravityOverride)
	_ARTAG(m_bFixed)
	_ARTAG(m_friction_mu_)
	_ARTAG(m_linear_momentum_)
	_ARTAG(m_angular_momentum_)
	_ARTAG(m_linear_friction_)
	_ARTAG(m_angular_friction_)
	_ARTAG(m_drag_force_)
	_ARTAG(m_force_)
	_ARTAG(m_torque_)
	_ARTAG(m_main_collider_)
)

namespace Engine::Component
{
	void Rigidbody::Initialize()
	{
		Component::Initialize();

		if (!GetOwner().lock()->GetComponent<Transform>().lock())
		{
			throw std::exception("Rigidbody must have a transform component");
		}

		if (!GetOwner().lock()->GetComponent<Collider>().lock())
		{
			throw std::exception("Rigidbody must have a collider component");
		}
	}

	void Rigidbody::SetMainCollider(const WeakCollider& collider)
	{
		m_main_collider_ = collider.lock()->GetLocalID();
	}

	WeakCollider Rigidbody::GetMainCollider() const
	{
		if (m_main_collider_ == 0)
		{
			return GetOwner().lock()->GetComponent<Collider>();
		}

		return GetOwner().lock()->GetComponentByLocal<Collider>(m_main_collider_);
	}

	void Rigidbody::Reset()
	{
		m_bGrounded = false;

		m_force_ = Vector3::Zero;
		m_torque_ = Vector3::Zero;
		m_drag_force_ = Vector3::Zero;
		m_linear_friction_ = Vector3::Zero;
		m_angular_friction_ = Vector3::Zero;
	}

	void Rigidbody::PreUpdate(const float& dt)
	{
	}

	void Rigidbody::Update(const float& dt)
	{
	}

	void Rigidbody::PreRender(const float dt)
	{
	}

	void Rigidbody::Render(const float dt)
	{
	}

	void Rigidbody::FixedUpdate(const float& dt)
	{
	}

	void Rigidbody::OnDeserialized()
	{
		Component::OnDeserialized();
	}

	void Rigidbody::OnImGui()
	{
		Component::OnImGui();

		ImGui::Indent(2);
		ImGui::Checkbox("Rigidbody Grounded", &m_bGrounded);
		ImGui::Checkbox("Rigidbody Gravity Override", &m_bGravityOverride);
		ImGui::Checkbox("Rigidbody Fixed", &m_bFixed);

		ImGui::DragFloat("Rigidbody Friction", &m_friction_mu_, 0.01f, 0, 1);

		ImGui::Text("Linear Momentum");
		ImGuiVector3Editable(GetID(), "linear_momentum", m_linear_momentum_);
		ImGui::Text("Angular Momentum");
		ImGuiVector3Editable(GetID(), "angular_momentum", m_angular_momentum_);

		ImGui::Text("Linear Friction");
		ImGuiVector3Editable(GetID(), "linear_friction", m_linear_friction_);

		ImGui::Text("Angular Friction");
		ImGuiVector3Editable(GetID(), "angular_friction", m_angular_friction_);
		ImGui::Text("Drag Force");
		ImGuiVector3Editable(GetID(), "drag_force", m_drag_force_);

		ImGui::Text("Rigidbody Force");
		ImGuiVector3Editable(GetID(), "force", m_force_);
		ImGui::Text("Rigidbody Torque");
		ImGuiVector3Editable(GetID(), "torque", m_torque_);

		ImGui::Text("Rigidbody Main Collider: %d", m_main_collider_);
		ImGui::Unindent(2);
	}

	Rigidbody::Rigidbody() : Component(COMPONENT_PRIORITY_RIGIDBODY, {}), m_bGrounded(false), m_bGravityOverride(false),
	                         m_bFixed(false),
	                         m_friction_mu_(0),
	                         m_main_collider_(0)
	{
	}
}
