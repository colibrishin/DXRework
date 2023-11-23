#include "pch.hpp"
#include "egPhysicsManager.hpp"
#include "egSceneManager.hpp"
#include "egCollider.hpp"
#include "egCollision.h"
#include "egElastic.h"
#include "egRigidbody.hpp"
#include "egFriction.h"

namespace Engine::Manager::Physics
{
	void PhysicsManager::Initialize()
	{
	}

	void PhysicsManager::PreUpdate(const float& dt)
	{
	}

	void PhysicsManager::Update(const float& dt)
	{
	}

	void PhysicsManager::PreRender(const float& dt)
	{
	}

	void PhysicsManager::Render(const float& dt)
	{
	}

	void PhysicsManager::FixedUpdate(const float& dt)
	{
		if (const auto scene = Engine::GetSceneManager().GetActiveScene().lock())
		{
			for (int i = 0; i < LAYER_MAX; ++i)
			{
				for (const auto& object : scene->GetGameObjects((eLayerType)i))
				{
					if (const auto obj = object.lock())
					{
						const auto tr = obj->GetComponent<Component::Transform>().lock();
						const auto rb = obj->GetComponent<Component::Rigidbody>().lock();
						const auto cl = obj->GetComponent<Component::Collider>().lock();

						if (!rb)
						{
							continue;
						}

						if (!cl->GetSpeculation().empty())
						{
							continue;
						}

						UpdateGravity(rb.get());
						UpdateObject(rb.get(), dt);
					}
				}
			}
		}
	}

	void PhysicsManager::UpdateGravity(Engine::Component::Rigidbody* rb)
	{
		if (rb->IsFixed() || !rb->IsGravityAllowed())
		{
			return;
		}

		const auto cl = rb->GetOwner().lock()->GetComponent<Component::Collider>().lock();

		if (!rb->IsGrounded())
		{
			rb->AddForce(Engine::Physics::g_gravity_vec * cl->GetInverseMass());
		}
		else
		{
			rb->AddForce(Vector3::Zero);
		}
	}

	void PhysicsManager::EpsilonGuard(Vector3& linear_momentum)
	{
		if (linear_momentum.Length() < g_epsilon)
		{
			linear_momentum = Vector3::Zero;
		}
	}

	void PhysicsManager::UpdateObject(Component::Rigidbody* rb, const float& dt)
	{
		if (rb->IsFixed())
		{
			return;
		}

		const auto cl = rb->GetOwner().lock()->GetComponent<Component::Collider>().lock();
		const auto tr = rb->GetOwner().lock()->GetComponent<Component::Transform>().lock();

		Vector3 linear_momentum = rb->GetLinearMomentum() + (rb->GetForce() * cl->GetInverseMass() * dt);
		const Vector3 linear_friction = Engine::Physics::EvalFriction(linear_momentum, rb->GetFrictionCoefficient(), dt);
		//const Vector3 drag_force = Engine::Physics::EvalDrag(linear_momentum, dt);

		const Vector3 angular_momentum = rb->GetAngularMomentum() + rb->GetTorque() * cl->GetInverseMass() * dt;

		rb->SetLinearFriction(linear_friction);
		linear_momentum += linear_friction;
		Engine::Physics::FrictionVelocityGuard(linear_momentum, linear_friction);

		//rb->SetDragForce(drag_force);
		//linear_momentum += drag_force;
		//Engine::Physics::FrictionVelocityGuard(linear_momentum, drag_force);

		EpsilonGuard(linear_momentum);

		rb->SetLinearMomentum(linear_momentum);
		rb->SetAngularMomentum(angular_momentum);

		tr->SetPosition(tr->GetPosition() + linear_momentum);

		//Quaternion orientation = tr->GetRotation();
		//orientation += Quaternion{angular_momentum * dt * 0.5f, 0.0f} * orientation;
		//orientation.Normalize();

		//tr->SetRotation(orientation);

		rb->Reset();
	}
}
