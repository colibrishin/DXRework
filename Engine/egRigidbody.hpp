#pragma once
#include "egCommon.hpp"
#include "egComponent.hpp"
namespace Engine::Component
{
	class Rigidbody : public Abstract::Component
	{
	public:
		explicit Rigidbody(const WeakObject& object) : Abstract::Component(COMPONENT_PRIORITY_RIGIDBODY, object),
														m_bGrounded(false), m_bGravityOverride(false),
														m_bFixed(false), m_friction_mu_(0.0f)
		{
		}

		~Rigidbody() override = default;

		void SetMainCollider(const WeakCollider& collider);
		void SetGravityOverride(bool gravity) { m_bGravityOverride = gravity;}
		void SetGrounded(bool grounded) { m_bGrounded = grounded;}

		void SetFrictionCoefficient(float mu) { m_friction_mu_ = mu; }
		void SetFixed(bool fixed) { m_bFixed = fixed; }

		void SetLinearMomentum(const Vector3& velocity) { m_linear_momentum_ = velocity; }
		void SetAngularMomentum(const Vector3& velocity) { m_angular_momentum_ = velocity; }
		void SetLinearFriction(const Vector3& friction) { m_linear_friction_ = friction; }
		void SetAngularFriction(const Vector3& friction) { m_angular_friction_ = friction; }
		void SetDragForce(const Vector3& drag) { m_drag_force_ = drag; }

		void AddLinearMomentum(const Vector3& velocity) { m_linear_momentum_ += velocity; }
		void AddAngularMomentum(const Vector3& velocity) { m_angular_momentum_ += velocity; }
		void AddForce(const Vector3& force) { m_force_ += force; }
		void AddTorque(const Vector3& torque) { m_torque_ += torque; }

		float GetFrictionCoefficient() const { return m_friction_mu_; }

		WeakCollider GetMainCollider() const;
		Vector3 GetLinearMomentum() const { return m_linear_momentum_; }
		Vector3 GetAngularMomentum() const { return m_angular_momentum_; }
		Vector3 GetForce() const { return m_force_; }
		Vector3 GetTorque() const { return m_torque_; }
		bool GetGrounded() const { return m_bGrounded; }

		void Reset();

		bool IsGravityAllowed() const { return m_bGravityOverride; }
		bool IsFixed() const { return m_bFixed; }
		bool IsGrounded() const { return m_bGrounded; }

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		TypeName GetVirtualTypeName() const final;

		void OnDeserialized() override;
		void OnImGui() override;

	protected:
		Rigidbody();

	private:
		SERIALIZER_ACCESS

		bool m_bGrounded;
		
		bool m_bGravityOverride;
		bool m_bFixed;

		float m_friction_mu_;

		Vector3 m_linear_momentum_;
		Vector3 m_angular_momentum_;

		Vector3 m_linear_friction_;
		Vector3 m_angular_friction_;
		Vector3 m_drag_force_;

		Vector3 m_force_;
		Vector3 m_torque_;

		ComponentID m_main_collider_;

	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Component::Rigidbody)