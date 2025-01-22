#ifdef PHYSX_ENABLED
#include <PxRigidDynamic.h>
#endif

#include "../Public/Rigidbody.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.hpp"
#include "Source/Runtime/Core/Components/Collider/Public/Collider.hpp"
#include "Source/Runtime/Core/Components/Transform/Public/Transform.h"
#include "Source/Runtime/MathExtension/Public/MathExtension.hpp"

SERIALIZE_IMPL
(
 Engine::Components::Rigidbody,
 _ARTAG(_BSTSUPER(Engine::Abstracts::Component)) _ARTAG(m_bGrounded)
 _ARTAG(m_bGravityOverride) _ARTAG(m_bFixed) _ARTAG(m_friction_mu_)
 _ARTAG(m_linear_velocity) _ARTAG(m_angular_velocity)
 _ARTAG(m_linear_friction_) _ARTAG(m_angular_friction_)
 _ARTAG(m_drag_force_) _ARTAG(m_t0_force_) _ARTAG(m_t0_torque_)
 _ARTAG(m_t1_force_) _ARTAG(m_t1_torque_)
)

namespace Engine::Components
{
	COMP_CLONE_IMPL(Rigidbody)

	void Rigidbody::Initialize()
	{
		Component::Initialize();

		if (!GetOwner().lock()->GetComponent<Transform>().lock())
		{
			GetOwner().lock()->AddComponent<Transform>();
		}

		if (!GetOwner().lock()->GetComponent<Collider>().lock())
		{
			GetOwner().lock()->AddComponent<Collider>();
		}

		SetGravityOverride(false);
		SetFixed(false);
		SetNoAngular(false);

#ifdef PHYSX_ENABLED
		if constexpr (g_speculation_enabled)
		{
			if (const StrongObjectBase& owner = GetOwner().lock())
			{
				if (const StrongCollider& collider = owner->GetComponent<Collider>().lock())
				{
					collider->m_px_rb_static_->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, true);
				}
			}
		}
#endif

		Synchronize();

		// todo/refactor: component dependency
		if (const Strong<Engine::Abstracts::ObjectBase>& owner = GetOwner().lock())
		{
			owner->onComponentRemoved.Listen(GetSharedPtr<Rigidbody>(), &Rigidbody::CheckColliderDependency);
		}
	}

	Rigidbody::Rigidbody(const Weak<Engine::Abstracts::ObjectBase>& object)
		: Component(COM_T_RIDIGBODY, object),
		  m_bGrounded(false),
		  m_b_no_angular_(false),
		  m_bGravityOverride(false),
		  m_bFixed(false),
		  m_b_lerp_(true),
		  m_friction_mu_(0.0f) {}

	Rigidbody::Rigidbody(const Rigidbody& other)
		: Component(other)
	{
		m_bGrounded         = other.m_bGrounded;
		m_b_no_angular_     = other.m_b_no_angular_;
		m_bGravityOverride  = other.m_bGravityOverride;
		m_bFixed            = other.m_bFixed;
		m_b_lerp_           = other.m_b_lerp_;
		m_friction_mu_      = other.m_friction_mu_;
		m_linear_velocity   = other.m_linear_velocity;
		m_angular_velocity  = other.m_angular_velocity;
		m_linear_friction_  = other.m_linear_friction_;
		m_angular_friction_ = other.m_angular_friction_;
		m_drag_force_       = other.m_drag_force_;
		m_t0_force_         = other.m_t0_force_;
		m_t0_torque_        = other.m_t0_torque_;
		m_t1_force_         = other.m_t1_force_;
		m_t1_torque_        = other.m_t1_torque_;
		// ignore t1 transform
	}

	Transform* Rigidbody::GetT1() const
	{
		return m_t1_.get();
	}

	void Rigidbody::SetGravityOverride(bool gravity)
	{
		m_bGravityOverride = gravity;

#ifdef PHYSX_ENABLED
		if (const StrongObjectBase& owner = GetOwner().lock())
		{
			if (const StrongCollider& collider = owner->GetComponent<Collider>().lock())
			{
				collider->GetPhysXRigidbody()->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, !gravity);
			}
		}
#endif
	}

	void Rigidbody::SetGrounded(bool grounded)
	{
		m_bGrounded = grounded;
	}

	void Rigidbody::SetFrictionCoefficient(float mu)
	{
		m_friction_mu_ = mu;
	}

	void Rigidbody::SetFixed(bool fixed)
	{
		m_bFixed = fixed;
#ifdef PHYSX_ENABLED
		if (const StrongObjectBase& owner = GetOwner().lock())
		{
			if (const StrongCollider& collider = owner->GetComponent<Collider>().lock())
			{
				collider->GetPhysXRigidbody()->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, fixed);
			}
		}
#endif
	}

	void Rigidbody::SetNoAngular(bool no_angular)
	{
		m_b_no_angular_ = no_angular;
#ifdef PHYSX_ENABLED
		if (const StrongObjectBase& owner = GetOwner().lock())
		{
			if (const StrongCollider& collider = owner->GetComponent<Collider>().lock())
			{
				collider->GetPhysXRigidbody()->setRigidDynamicLockFlags(
					physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X | 
					physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y | 
					physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z);
			}
		}
#endif
	}

	void Rigidbody::Synchronize()
	{
		m_t1_ = std::make_unique<Transform>(*GetOwner().lock()->GetComponent<Transform>().lock());
	}

	void Rigidbody::SetT0LinearVelocity(const Vector3& v)
	{
		m_linear_velocity = v;

#ifdef PHYSX_ENABLED
		if (const StrongObjectBase& owner = GetOwner().lock())
		{
			if (const StrongCollider& collider = owner->GetComponent<Collider>().lock())
			{
				collider->GetPhysXRigidbody()->setLinearVelocity(
					reinterpret_cast<const physx::PxVec3&>(m_linear_velocity));
			}
		}
#endif
	}

	void Rigidbody::SetT0AngularVelocity(const Vector3& v)
	{
		m_angular_velocity = v;

#ifdef PHYSX_ENABLED
		if (const StrongObjectBase& owner = GetOwner().lock())
		{
			if (const StrongCollider& collider = owner->GetComponent<Collider>().lock())
			{
				collider->GetPhysXRigidbody()->setLinearVelocity(
					reinterpret_cast<const physx::PxVec3&>(m_angular_velocity));
			}
		}
#endif
	}

	void Rigidbody::AddLinearImpulse(const Vector3& f)
	{
		if (m_bFixed)
		{
			return;
		}
		MathExtension::Vector3CheckNanException(f);
		m_linear_velocity += f;
	}

	void Rigidbody::AddAngularImpulse(const Vector3& f)
	{
		if (m_bFixed)
		{
			return;
		}
		MathExtension::Vector3CheckNanException(f);
		m_angular_velocity += f;
	}

	void Rigidbody::SetLinearFriction(const Vector3& friction)
	{
		m_linear_friction_ = friction;
	}

	void Rigidbody::SetAngularFriction(const Vector3& friction)
	{
		m_angular_friction_ = friction;
	}

	void Rigidbody::SetDragForce(const Vector3& drag)
	{
		m_drag_force_ = drag;
	}

	void Rigidbody::AddT1Force(const Vector3& force)
	{
		m_t1_force_ += force;

#ifdef PHYSX_ENABLED
		if (const StrongObjectBase& owner = GetOwner().lock())
		{
			if (const StrongCollider& collider = owner->GetComponent<Collider>().lock())
			{
				collider->GetPhysXRigidbody()->addForce(reinterpret_cast<const physx::PxVec3&>(force));
			}
		}
#endif
	}

	void Rigidbody::AddT1Torque(const Vector3& torque)
	{
		m_t1_torque_ += torque;

#ifdef PHYSX_ENABLED
		if (const StrongObjectBase& owner = GetOwner().lock())
		{
			if (const StrongCollider& collider = owner->GetComponent<Collider>().lock())
			{
				collider->GetPhysXRigidbody()->addTorque(reinterpret_cast<const physx::PxVec3&>(torque));
			}
		}
#endif
	}

	float Rigidbody::GetFrictionCoefficient() const
	{
		return m_friction_mu_;
	}

	Vector3 Rigidbody::GetT0LinearVelocity() const
	{
		return m_linear_velocity;
	}

	Vector3 Rigidbody::GetT0AngularVelocity() const
	{
		return m_angular_velocity;
	}

	Vector3 Rigidbody::GetT0Force() const
	{
		return m_t0_force_;
	}

	Vector3 Rigidbody::GetT0Torque() const
	{
		return m_t0_torque_;
	}

	Vector3 Rigidbody::GetT1Force() const
	{
		return m_t1_force_;
	}

	Vector3 Rigidbody::GetT1Torque() const
	{
		return m_t1_torque_;
	}

	bool Rigidbody::GetGrounded() const
	{
		return m_bGrounded;
	}

	bool Rigidbody::IsGravityAllowed() const
	{
		return m_bGravityOverride;
	}

	bool Rigidbody::IsFixed() const
	{
		return m_bFixed;
	}

	bool Rigidbody::IsGrounded() const
	{
		return m_bGrounded;
	}

	bool Rigidbody::GetNoAngular() const
	{
		return m_b_no_angular_;
	}

	bool Rigidbody::GetLerp() const
	{
		return m_b_lerp_;
	}

	void Rigidbody::Reset()
	{
		m_bGrounded = false;

		m_t0_force_         = m_t1_force_;
		m_t0_torque_        = m_t1_torque_;
		m_t1_force_         = Vector3::Zero;
		m_t1_torque_        = Vector3::Zero;
		m_drag_force_       = Vector3::Zero;
		m_linear_friction_  = Vector3::Zero;
		m_angular_friction_ = Vector3::Zero;
	}

	void Rigidbody::FullReset()
	{
		m_bGrounded = false;

		m_linear_velocity   = Vector3::Zero;
		m_angular_velocity  = Vector3::Zero;
		m_t0_force_         = Vector3::Zero;
		m_t0_torque_        = Vector3::Zero;
		m_t1_force_         = Vector3::Zero;
		m_t1_torque_        = Vector3::Zero;
		m_drag_force_       = Vector3::Zero;
		m_linear_friction_  = Vector3::Zero;
		m_angular_friction_ = Vector3::Zero;

		Synchronize();
	}

	void Rigidbody::PreUpdate(const float& dt) {}

	void Rigidbody::Update(const float& dt) {}

	void Rigidbody::PostUpdate(const float& dt)
	{
		Component::PostUpdate(dt);
	}

	void Rigidbody::FixedUpdate(const float& dt) {}

	void Rigidbody::OnSerialized()
	{
		Component::OnSerialized();
	}

	void Rigidbody::OnDeserialized()
	{
		Component::OnDeserialized();
	}

	Rigidbody::Rigidbody()
		: Component(COM_T_RIDIGBODY, {}),
		  m_bGrounded(false),
		  m_b_no_angular_(false),
		  m_bGravityOverride(false),
		  m_bFixed(false),
		  m_b_lerp_(true),
		  m_friction_mu_(0) {}

	void Rigidbody::CheckColliderDependency(Weak<Component> component) const
	{
		if (const Strong<Component>& locked = component.lock())
		{
			if (locked->GetComponentType() == COM_T_COLLIDER)
			{
				if (const Strong<Engine::Abstracts::ObjectBase>& owner = GetOwner().lock())
				{
					owner->RemoveComponent<Rigidbody>();
				}
			}
		}
	}

	void Rigidbody::SetT1Force(const Vector3& force)
	{
		m_t1_force_ = force;

#ifdef PHYSX_ENABLED
		if (const StrongObjectBase& owner = GetOwner().lock())
		{
			if (const StrongCollider& collider = owner->GetComponent<Collider>().lock())
			{
				collider->GetPhysXRigidbody()->setForceAndTorque(reinterpret_cast<const physx::PxVec3&>(force), reinterpret_cast<const physx::PxVec3&>(m_t1_torque_));
			}
		}
#endif
	}

	void Rigidbody::SetT1Torque(const Vector3& torque)
	{
		m_t1_torque_ = torque;

#ifdef PHYSX_ENABLED
		if (const StrongObjectBase& owner = GetOwner().lock())
		{
			if (const StrongCollider& collider = owner->GetComponent<Collider>().lock())
			{
				collider->GetPhysXRigidbody()->setForceAndTorque(reinterpret_cast<const physx::PxVec3&>(m_t1_force_), reinterpret_cast<const physx::PxVec3&>(torque));
			}
		}
#endif
	}
} // namespace Engine::Component
