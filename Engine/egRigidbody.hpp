#pragma once
#include "egCommon.hpp"
#include "egComponent.hpp"
#include "egLayer.hpp"
#include "egTransform.hpp"

namespace Engine::Component
{
	constexpr Vector3 g_gravity_vec = Vector3(0, -9.8f, 0);

	class Rigidbody : public Abstract::Component
	{
	public:
		explicit Rigidbody(const WeakObject& object) : Abstract::Component(COMPONENT_PRIORITY_RIGIDBODY, object),
														m_bFreefalling(false), m_bInternalVOverride(false),
														m_bGravityOverride(false), m_bElastic(true), m_mass_(1.0f),
														m_kinetic_friction(0),
														m_static_friction(0),
														m_xz_friction_(),
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
		void SetFreefalling(bool gravity) { m_bFreefalling = gravity; }
		void SetElastic(bool elastic) { m_bElastic = elastic; }
		void SetMass(float mass) { m_mass_ = mass; }
		void SetInternalVelocityOverride(bool override) { m_bInternalVOverride = override; }

		void SetVelocity(const Vector3& force) { m_velocity_ = force; }
		void SetInternalVelocity(const Vector3& force) { m_velocity_internal_ = force; }
		void SetAcceleration(const Vector3& acceleration) { m_acceleration_ = acceleration; }

		float GetMass() const { return m_mass_; }
		bool GetElastic() const { return m_bElastic; }
		bool GetGravity() const { return m_bFreefalling; }
		Vector3 GetVelocity() const { return m_velocity_; }

		Vector3 GetVerlet() const
		{
			return (m_velocity_ + m_velocity_internal_) + ((m_acceleration_ + (IsFreefalling()
																					? g_gravity_vec
																					: Vector3::Zero)) * GetDeltaTime());
		}
		bool IsFreefalling() const { return m_bFreefalling && m_bGravityOverride; }

		void AddFrictionMap(const WeakObject& obj, const Vector3& friction)
		{
			if (m_friction_map_.contains(obj))
			{
				return;
			}

			m_xz_friction_ += friction;
			m_friction_map_.insert(std::make_pair(obj, friction));
		}

		void RemoveFrictionMap(const WeakObject& obj)
		{
			if (!m_friction_map_.contains(obj))
			{
				return;
			}

			m_xz_friction_ -= m_friction_map_.at(obj);
			m_friction_map_.erase(obj);
		}

		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;
		void FixedUpdate() override;

	private:
		static void ClampFriction(const Vector3& v0, Vector3& v1);
		static Vector3 GetPolarity(const Vector3& v)
		{
			return {std::copysign(1.0f, v.x), std::copysign(1.0f, v.y), std::copysign(1.0f, v.z)};
		}

		static Vector3 GetActiveForce(const Vector3& v)
		{
			return { v.x != 0.f ? 1.0f : 0.0f, v.y != 0.f ? 1.0f : 0.0f, v.z != 0.f ? 1.0f : 0.0f};
		}

	private:
		bool m_bFreefalling;
		bool m_bInternalVOverride;
		bool m_bGravityOverride;
		bool m_bElastic;

		float m_mass_;

		float m_kinetic_friction;
		float m_static_friction;

		Vector3 m_velocity_;
		Vector3 m_velocity_internal_;
		Vector3 m_acceleration_;

		Vector3 m_xz_friction_;

		std::map<WeakObject, Vector3, WeakObjComparer> m_friction_map_;

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

	inline void Rigidbody::ClampFriction(const Vector3& v0, Vector3& v1)
	{
		if (!IsSamePolarity(v0.x, v1.x))
		{
			v1.x = 0.0f;
		}
		else if (!IsSamePolarity(v0.y, v1.y))
		{
			v1.y = 0.0f;
		}
		else if (!IsSamePolarity(v0.z, v1.z))
		{
			v1.z = 0.0f;
		}
	}

	inline void Rigidbody::Update()
	{
		const auto tr = GetOwner().lock()->GetComponent<Transform>().lock();

		auto force = GetVerlet();

		if (m_bInternalVOverride && m_velocity_ != Vector3::Zero)
		{
			force -= m_velocity_internal_;
		}

		tr->SetPosition(tr->GetPosition() + force);

		if (m_velocity_internal_.Length() >= 0.f)
		{
			auto reduced = m_velocity_internal_ - Vector3::One * GetDeltaTime() * GetDeltaTime();

			if (!IsSamePolarity(reduced.x, m_velocity_internal_.x))
			{
				reduced.x = 0.f;
			}
			if (!IsSamePolarity(reduced.y, m_velocity_internal_.y))
			{
				reduced.y = 0.f;
			}
			if (!IsSamePolarity(reduced.z, m_velocity_internal_.z))
			{
				reduced.z = 0.f;
			}

			m_velocity_internal_ = reduced;
		}
	}

	inline void Rigidbody::PreRender()
	{
	}

	inline void Rigidbody::Render()
	{
	}

	inline void Rigidbody::FixedUpdate()
	{
	}
}
