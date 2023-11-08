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
					Vector3 normal;
					float penetration;

					m_bFreefalling = false;
					collider->GetPenetration(*other_collider, normal, penetration);

					const auto tr = GetOwner().lock()->GetComponent<Transform>().lock();

					tr->SetPosition(tr->GetPosition() + (normal * penetration));

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
					m_friction_ += Physics::EvalFriction(m_linear_momentum_, other_rb->GetFrictionCoefficient(), GetDeltaTime());
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
				m_gravity_ = Physics::EvalGravity(collider->GetInverseMass(), GetDeltaTime());
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

		m_linear_momentum_ = Physics::EvalVerlet(m_linear_momentum_, m_force_, GetDeltaTime());

		EvaluateFriction();
	}

	void Rigidbody::FrictionVelocityGuard(Vector3& evaluated_velocity, const Vector3& friction) const
	{
		const Vector3 Undo = evaluated_velocity - friction;

		if (!IsSamePolarity(evaluated_velocity.x, Undo.x))
		{
			evaluated_velocity.x = 0.0f;
		}
		if (!IsSamePolarity(evaluated_velocity.y, Undo.y))
		{
			evaluated_velocity.y = 0.0f;
		}
		if (!IsSamePolarity(evaluated_velocity.z, Undo.z))
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

		m_linear_momentum_ += m_friction_;
		FrictionVelocityGuard(m_linear_momentum_, m_friction_);

		tr->SetPosition(tr->GetPosition() + m_linear_momentum_ + m_gravity_);

		m_force_ = Vector3::Zero;
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
