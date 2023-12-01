#include "pch.hpp"
#include "egRigidbody.hpp"
#include "egCollider.hpp"
#include "egKinetic.h"
#include "egScene.hpp"

namespace Engine::Component
{
	void Rigidbody::Initialize_INTERNAL()
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

	WeakCollider Rigidbody::GetMainCollider() const
	{
		if (!m_main_collider_.lock())
		{
			return GetOwner().lock()->GetComponent<Collider>();
		}

		return m_main_collider_;
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

	void Rigidbody::OnLayerChanging()
	{
	}

	void Rigidbody::OnLayerChanged()
	{
	}

	void Rigidbody::OnCreate()
	{
		if (const auto scene = GetOwner().lock()->GetScene().lock())
		{
			scene->AddComponent<Rigidbody>(GetSharedPtr<Rigidbody>());
		}
	}

	void Rigidbody::OnDestroy()
	{
		if (const auto scene = GetOwner().lock()->GetScene().lock())
		{
			scene->RemoveComponent<Rigidbody>(GetSharedPtr<Rigidbody>());
		}
	}

	void Rigidbody::OnSceneChanging()
	{
		OnDestroy();
	}

	void Rigidbody::OnSceneChanged()
	{
		OnCreate();
	}
}
