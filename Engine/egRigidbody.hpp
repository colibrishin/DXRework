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
		using CollisionCountPair = std::pair<uint64_t, UINT>;

		explicit Rigidbody(const WeakObject& object) : Abstract::Component(COMPONENT_PRIORITY_RIGIDBODY, object),
														m_bGrounded(false), m_bGravityOverride(false),
														m_bFixed(false), m_friction_mu_(0.0f)
		{
		}

		~Rigidbody() override = default;

		void Initialize() override;

		void SetGravityOverride(bool gravity) { m_bGravityOverride = gravity;}

		void SetFrictionCoefficient(float mu) { m_friction_mu_ = mu; }
		void SetFixed(bool fixed) { m_bFixed = fixed; }
		void SetGrounded(bool grounded) { m_bGrounded = grounded;}

		void AddForce(const Vector3& acceleration) { m_force_ += acceleration; }
		void AddForceAtPosition(const Vector3& acceleration, const Vector3& position);
		void AddLinearMomentum(const Vector3& momentum);
		void AddAngularMomentum(const Vector3& momentum);
		void AddCollisionCount(uint64_t id) { m_collision_count_[id]++; }

		void SetForceAtPosition(const Vector3& acceleration, const Vector3& position);

		float GetFrictionCoefficient() const { return m_friction_mu_; }
		Vector3 GetGravity() const { return m_gravity_; }
		Vector3 GetLinearMomentum() const { return m_linear_momentum_; }
		Vector3 GetAngularMomentum() const { return m_angular_momentum_; }
		Vector3 GetForce() const { return m_force_; }
		UINT GetCollisionCount(uint64_t id) const
		{
			if (!m_collision_count_.contains(id))
			{
				return 0;
			}

			return m_collision_count_.at(id);
		}

		bool IsGravityAllowed() const { return m_bGravityOverride; }
		bool IsFixed() const { return m_bFixed; }
		bool IsGrounded() const { return m_bGrounded; }
		bool HasGroundedChangedInFrame() const { return m_bGrounded != m_bPreviousGrounded; }

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void FixedUpdate(const float& dt) override;

	private:
		void EvaluateFriction(const float dt);
		void FrictionVelocityGuard(Vector3& evaluated_velocity, const Vector3& friction) const;

		std::map<uint64_t, UINT> m_previous_collision_count_;
		std::map<uint64_t, UINT> m_collision_count_;

		// Ground check flag for CollisionDetector
		bool m_bPreviousGrounded;
		bool m_bGrounded;
		
		bool m_bGravityOverride;
		bool m_bFixed;

		float m_friction_mu_;

		Vector3 m_previous_position_;
		Vector3 m_next_position_;

		Vector3 m_linear_momentum_;
		Vector3 m_angular_momentum_;

		Vector3 m_linear_friction_;
		Vector3 m_angular_friction_;
		Vector3 m_drag_force_;

		Vector3 m_gravity_;

		Vector3 m_force_;
		Vector3 m_torque_;

	};
}
