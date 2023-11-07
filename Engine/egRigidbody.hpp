#pragma once
#include "egCommon.hpp"
#include "egComponent.hpp"
#include "egLayer.hpp"
#include "egTransform.hpp"
#include "egPhysics.h"

namespace Engine::Component
{
	class Rigidbody : public Abstract::Component
	{
	public:
		explicit Rigidbody(const WeakObject& object) : Abstract::Component(COMPONENT_PRIORITY_RIGIDBODY, object),
														m_bFreefalling(true), m_bGravityOverride(false),
														m_bFixed(false), m_friction_mu_(0.0f),
														m_elasticity_(1.0f),
														m_velocity_(Vector3::Zero),
														m_acceleration_(Vector3::Zero)
		{
		}

		~Rigidbody() override = default;

		void Initialize() override;

		void SetGravityOverride(bool gravity)
		{
			m_bGravityOverride = gravity;
			m_bFreefalling = gravity;
		}

		void SetFrictionCoefficient(float mu) { m_friction_mu_ = mu; }
		void SetFixed(bool fixed) { m_bFixed = fixed; }
		void SetElasticity(float elasticity) { m_elasticity_ = elasticity; }
		void SetAcceleration(const Vector3& acceleration) { m_acceleration_ = acceleration; }
		void SetLinearMomentum(const Vector3& momentum) { m_linear_momentum_ = momentum; }
		void SetAngularMomentum(const Vector3& momentum) { m_angular_momentum_ = momentum; }

		float GetFrictionCoefficient() const { return m_friction_mu_; }
		Vector3 GetGravity() const { return m_gravity_; }
		Vector3 GetVelocity() const { return m_velocity_; }
		float GetElasticity() const { return m_elasticity_; }
		Vector3 GetAcceleration() const { return m_acceleration_; }

		bool IsGravityAllowed() const { return m_bGravityOverride; }
		bool IsFreefalling() const { return m_bFreefalling && m_bGravityOverride; }
		bool IsFixed() const { return m_bFixed; }

		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;
		void FixedUpdate() override;

	private:
		void CheckFloorForGravity();
		void EvaluateFriction();

		void FrictionVelocityGuard(Vector3& evaluated_velocity) const;

		bool m_bFreefalling;
		bool m_bGravityOverride;
		bool m_bFixed;

		float m_friction_mu_;
		float m_elasticity_;

		Vector3 m_linear_momentum_;
		Vector3 m_angular_momentum_;

		Vector3 m_previous_velocity_;
		Vector3 m_velocity_;
		Vector3 m_friction_;
		Vector3 m_gravity_;
		Vector3 m_acceleration_;
		Vector3 m_net_force_;

	};
}
