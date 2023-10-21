#pragma once
#include "egComponent.hpp"
#include "egLayer.hpp"
#include "egTransform.hpp"

namespace Engine::Component
{
	class Rigidbody : public Abstract::Component
	{
	public:
		explicit Rigidbody(const WeakObject& object) : Abstract::Component(object), m_bGravity(false), m_mass(1.0f),
		                                               m_base_friction(0.0f), m_env_friction(), m_initial_velocity(Vector3::Zero), m_Acceleration(Vector3::Zero)
		{
		}

		~Rigidbody() override = default;

		void Initialize() override;

		void SetGravity(bool gravity) { m_bGravity = gravity; }
		void SetMass(float mass) { m_mass = mass; }
		void SetFriction(float friction) { m_base_friction = friction; }

		void SetVelocity(const Vector3& force) { m_initial_velocity = force; }
		void SetAcceleration(const Vector3& acceleration) { m_Acceleration = acceleration; }

		float GetFriction() const { return m_base_friction; }

		void AddFriction(const Vector3& friction) { m_env_friction = friction; }

		void SubtractFriction(const Vector3& friction) { m_env_friction -= friction; }

		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;

	private:
		bool m_bGravity;

		float m_mass;
		float m_base_friction;

		Vector3 m_env_friction;
		Vector3 m_initial_velocity;
		Vector3 m_Acceleration;

	};

	inline void Rigidbody::Initialize()
	{
		if (!GetOwner().lock()->GetComponent<Transform>().lock())
		{
			throw std::exception("Rigidbody must have a transform component");
		}
	}

	inline void Rigidbody::PreUpdate()
	{
	}

	inline void Rigidbody::Update()
	{
		const auto tr = GetOwner().lock()->GetComponent<Transform>().lock();

		const auto velocity_over_time = (m_initial_velocity + (m_Acceleration));
		const auto kinetic = ((m_mass * 0.5f) * (velocity_over_time * velocity_over_time));
		const auto friction = (m_env_friction * m_mass * g_gravity_acc) * Vector3{1.0f, 0.0f, 1.0f};

		const Vector3 velocity_polarity = { std::copysign(1.f, velocity_over_time.x), std::copysign(1.f, velocity_over_time.y), std::copysign(1.f, velocity_over_time.z) };
		const Vector3 active_force = velocity_over_time / velocity_over_time;

		Vector3 velocity = (kinetic * velocity_polarity) + ((friction) * -velocity_polarity * active_force);

		if (std::copysign(1.f, velocity.x) != std::copysign(1.f, velocity_over_time.x))
		{
			velocity.x = 0;
		}
		if (std::copysign(1.f, velocity.y) != std::copysign(1.f, velocity_over_time.y))
		{
			velocity.y = 0;
		}
		if (std::copysign(1.f, velocity.z) != std::copysign(1.f, velocity_over_time.z))
		{
			velocity.z = 0;
		}

		const auto gravity = Vector3(0, g_gravity_acc, 0) * GetDeltaTime();

		tr->SetPosition(tr->GetPosition() + (velocity * GetDeltaTime()) - (m_bGravity ? gravity : Vector3::Zero));
	}

	inline void Rigidbody::PreRender()
	{
	}

	inline void Rigidbody::Render()
	{
	}
}
