#include "pch.hpp"
#include "egRigidbody.hpp"
#include "egCollider.hpp"
#include "egKinetic.h"
#include "egScene.hpp"
#include "egCollider.hpp"

namespace Engine::Component
{
	void Rigidbody::Initialize()
	{
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

	void Rigidbody::AfterDeserialized()
	{
		// @todo: can weak collider be serialized as intended?
		throw new std::exception("Not implemented");
	}
}
