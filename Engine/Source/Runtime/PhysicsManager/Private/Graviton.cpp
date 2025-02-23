#include "Graviton.h"

#include "Friction.hpp"
#include "Verlet.hpp"

#include "Collider.h"
#include "Rigidbody.h"

#include "ObjectBase.h"

#include "Scene.h"

#include "SceneManager.h"

namespace Engine::Managers
{
	void Graviton::PreUpdate(const float dt) {}

	void Graviton::Update(const float dt) {}

	void Graviton::PostUpdate(const float dt) {}

	void Graviton::FixedUpdate(const float dt)
	{
		if (!SceneManager::GetInstance().IsPlaying())
		{
            return;
		}

		if (const auto scene = SceneManager::GetInstance().GetActiveScene().lock())
		{
			const auto& comps = scene->GetCachedComponents<Components::Rigidbody>();

			for (const auto& ptr_comp : comps)
			{
				if (const auto comp = ptr_comp.lock())
				{
					const auto& rb = comp->GetSharedPtr<Components::Rigidbody>();

					if (rb->IsFixed())
					{
						continue;
					}
					if (!rb->IsGravityAllowed())
					{
						continue;
					}
					if (!rb->GetActive())
					{
						continue;
					}

					const auto cl   = rb->GetOwner().lock()->GetComponent<Components::Collider>().lock();
					const auto drag = EvalDrag(rb->GetT0LinearVelocity(), CFG_DRAG_COEFFICIENT);

					rb->AddT1Force((g_gravity_vec * cl->GetInverseMass()) + (drag * cl->GetInverseMass()));
					rb->SetDragForce(drag);
				}
			}
		}
	}

	void Graviton::PreRender(const float dt) {}

	void Graviton::Render(const float dt) {}

	void Graviton::PostRender(const float dt) {}

	void Graviton::Initialize() {}
}
