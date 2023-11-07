#include "pch.hpp"
#include "egRigidbody.hpp"
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

	void Rigidbody::CheckFloorForGravity()
	{
		if (const auto collider = GetOwner().lock()->GetComponent<Collider>().lock())
		{
			const auto collided_objects = collider->GetCollidedObjects();

			// we have no objects to check against. gravity is applied.
			if (collided_objects.empty())
			{
				m_bFreefalling = true;
				return;
			}

			// floor check
			Collider copy = *collider;
			copy.SetPosition(collider->GetPosition() + Vector3{0.0f, -g_epsilon, 0.0f});

			for (const auto& id : collided_objects)
			{
				const auto obj = GetSceneManager().GetActiveScene().lock()->FindGameObject(id).lock();
				const auto other_collider = obj->GetComponent<Collider>().lock();

				// Collider must be present in the collided object
				assert(other_collider);

				// if any of the collided objects are intersecting by the floor, then we are not freefalling.
				if (const bool is_grounded = copy.Intersects(*other_collider))
				{
					m_bFreefalling = false;
					return;
				}
			}

			// if none of the collided objects are intersecting by the floor, then we are freefalling.
			m_bFreefalling = true;
		}
	}

	void Rigidbody::EvaluateFriction()
	{
		if (const auto collider = GetOwner().lock()->GetComponent<Collider>().lock())
		{
			const auto collided_objects = collider->GetCollidedObjects();

			// we have no objects to check against. friction is not applied.
			if (collided_objects.empty())
			{
				m_friction_ = Vector3::Zero;
			}

			for (const auto& id : collided_objects)
			{
				const auto obj = GetSceneManager().GetActiveScene().lock()->FindGameObject(id).lock();

				if (const auto other_rb = obj->GetComponent<Rigidbody>().lock())
				{
					m_friction_ += Physics::EvalFriction(m_velocity_, other_rb->GetFrictionCoefficient(), GetDeltaTime());
				}
			}
		}
	}

	void Rigidbody::PreUpdate()
	{
		const auto collider = GetOwner().lock()->GetComponent<Collider>().lock();

		if (m_bFixed)
		{
			return;
		}

		if (m_bGravityOverride)
		{
			CheckFloorForGravity();

			if (m_bFreefalling)
			{
				m_gravity_ = Physics::EvalGravity(collider->GetMass(), GetDeltaTime());
			}
			else
			{
				m_gravity_ = Vector3::Zero;
			}
		}
		else
		{
			m_gravity_ = Vector3::Zero;
		}

		EvaluateFriction();

		m_velocity_ = Physics::EvalVerlet(m_velocity_, m_acceleration_, GetDeltaTime());
	}

	void Rigidbody::FrictionVelocityGuard(Vector3& evaluated_velocity) const
	{
		if (!IsSamePolarity(evaluated_velocity.x, m_velocity_.x))
		{
			evaluated_velocity.x = 0.0f;
		}
		if (!IsSamePolarity(evaluated_velocity.y, m_velocity_.y))
		{
			evaluated_velocity.y = 0.0f;
		}
		if (!IsSamePolarity(evaluated_velocity.z, m_velocity_.z))
		{
			evaluated_velocity.z = 0.0f;
		}
	}

	void Rigidbody::Update()
	{
		if (m_bFixed)
		{
			return;
		}

		const auto tr = GetOwner().lock()->GetComponent<Transform>().lock();

		m_velocity_ += m_friction_ + m_linear_momentum_;
		FrictionVelocityGuard(m_velocity_);

		tr->SetPosition(tr->GetPosition() + m_velocity_ + m_gravity_);

		m_acceleration_ = Vector3::Zero;
		m_gravity_ = Vector3::Zero;
		m_friction_ = Vector3::Zero;
	}

	void Rigidbody::PreRender()
	{
	}

	void Rigidbody::Render()
	{
	}

	void Rigidbody::FixedUpdate()
	{
	}
}
