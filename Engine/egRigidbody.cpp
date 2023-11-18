#include "pch.hpp"
#include "egRigidbody.hpp"
#include "egCollider.hpp"
#include "egKinetic.h"

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
}
