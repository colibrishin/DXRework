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

	void Rigidbody::AddForceAtPosition(const Vector3& acceleration, const Vector3& position)
	{
		const auto cl = GetOwner().lock()->GetComponent<Collider>().lock();
		const auto tr = GetOwner().lock()->GetComponent<Transform>().lock();

		auto r = position - tr->GetPosition();
		r.Normalize();

		m_force_ += acceleration;
		m_torque_ += r.Cross(acceleration);
	}

	void Rigidbody::AddLinearMomentum(const Vector3& momentum)
	{
		const auto cl = GetOwner().lock()->GetComponent<Collider>().lock();
		m_linear_momentum_ += momentum * cl->GetInverseMass();
	}

	void Rigidbody::AddAngularMomentum(const Vector3& momentum)
	{
		const auto cl = GetOwner().lock()->GetComponent<Collider>().lock();
		m_angular_momentum_ += XMTensorCross(cl->GetInertiaTensor(), momentum);
	}

	void Rigidbody::SetForceAtPosition(const Vector3& acceleration, const Vector3& position)
	{
		const auto cl = GetOwner().lock()->GetComponent<Collider>().lock();
		const auto tr = GetOwner().lock()->GetComponent<Transform>().lock();

		auto r = position - tr->GetPosition();
		r.Normalize();

		m_force_ = acceleration;
		m_torque_ = r.Cross(acceleration);
	}

	void Rigidbody::EvaluateFriction(const float dt)
	{
		m_drag_force_ = Physics::EvalDrag(m_linear_momentum_, Physics::g_drag_coefficient);

		if (const auto collider = GetOwner().lock()->GetComponent<Collider>().lock())
		{
			const auto collided_objects = collider->GetCollidedObjects();

			// we have no objects to check against. friction is not applied.
			if (collided_objects.empty())
			{
				m_linear_friction_ = Vector3::Zero;
			}

			for (const auto& id : collided_objects)
			{
				const auto obj = GetSceneManager().GetActiveScene().lock()->FindGameObject(id).lock();

				if (const auto other_rb = obj->GetComponent<Rigidbody>().lock())
				{
					m_linear_friction_ += Physics::EvalFriction(m_linear_momentum_, other_rb->GetFrictionCoefficient(), dt);
					m_angular_friction_ += Physics::EvalFriction(m_angular_momentum_, other_rb->GetFrictionCoefficient(), dt);
				}
			}
		}
	}

	void Rigidbody::PreUpdate(const float& dt)
	{
		m_bPreviousGrounded = m_bGrounded;
		m_bGrounded = false;

		m_force_ = Vector3::Zero;
		m_torque_ = Vector3::Zero;
		m_gravity_ = Vector3::Zero;
		m_drag_force_ = Vector3::Zero;
		m_linear_friction_ = Vector3::Zero;
		m_angular_friction_ = Vector3::Zero;
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

	void Rigidbody::Update(const float& dt)
	{
		if (m_bFixed)
		{
			return;
		}

		const auto collider = GetOwner().lock()->GetComponent<Collider>().lock();
		const auto tr = GetOwner().lock()->GetComponent<Transform>().lock();

		if (m_bGravityOverride)
		{
			if (!m_bPreviousGrounded && m_bGrounded)
			{
				m_gravity_ = Physics::EvalGravity(collider->GetInverseMass(), dt);
			}
			else if (m_bGrounded)
			{
				m_gravity_ = Vector3::Zero;
			}
			else
			{
				m_gravity_ = Physics::EvalGravity(collider->GetInverseMass(), dt);
			}
		}
		else
		{
			m_gravity_ = Vector3::Zero;
		}

		m_linear_momentum_ = Physics::EvalVerlet(m_linear_momentum_, m_force_, dt);
		m_angular_momentum_ = Physics::EvalAngular(m_angular_momentum_, m_torque_, dt);

		EvaluateFriction(dt);

		m_linear_momentum_ += m_linear_friction_;
		FrictionVelocityGuard(m_linear_momentum_, m_linear_friction_);

		m_linear_momentum_ += m_drag_force_;
		FrictionVelocityGuard(m_linear_momentum_, m_drag_force_);

		m_linear_momentum_ += m_gravity_;

		//m_angular_momentum_ += m_angular_friction_;
		//FrictionVelocityGuard(m_angular_momentum_, m_angular_friction_);
	}

	void Rigidbody::PreRender(const float dt)
	{
		// after CollisionManager resolves collision, update the position lately.
		const auto tr = GetOwner().lock()->GetComponent<Transform>().lock();

		m_previous_position_ = tr->GetPosition();
		m_next_position_ = tr->GetPosition() + m_linear_momentum_;

		tr->SetPosition(m_next_position_);

		Quaternion orientation = tr->GetRotation();
		orientation += Quaternion{m_angular_momentum_ * dt * 0.5f, 0.0f} * orientation;
		orientation.Normalize();

		tr->SetRotation(orientation);
	}

	void Rigidbody::Render(const float dt)
	{
	}

	void Rigidbody::FixedUpdate(const float& dt)
	{
		m_previous_collision_count_ = m_collision_count_;
		m_collision_count_.clear();
	}
}
