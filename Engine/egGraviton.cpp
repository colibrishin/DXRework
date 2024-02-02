#include "pch.h"
#include "egGraviton.h"

#include "egBaseCollider.hpp"
#include "egRigidbody.h"
#include "egSceneManager.hpp"

namespace Engine::Manager::Physics
{
	void Graviton::PreUpdate(const float& dt) {}

	void Graviton::Update(const float& dt) {}

  void Graviton::PostUpdate(const float& dt) {}

  void Graviton::FixedUpdate(const float& dt)
  {
    if (const auto scene = GetSceneManager().GetActiveScene().lock())
    {
      const auto& comps = scene->GetCachedComponents<Components::Rigidbody>();

      for (const auto& ptr_comp : comps)
      {
        if (const auto comp = ptr_comp.lock())
        {
          const auto& rb = comp->GetSharedPtr<Components::Rigidbody>();

          if (rb->IsFixed()) { continue; }
          if (!rb->IsGravityAllowed()) { continue; }
          if (rb->GetGrounded()) { continue; }
          if (!rb->GetActive()) { continue; }

          const auto cl = rb->GetOwner().lock()->GetComponent<Components::Collider>().lock();

          rb->AddForce(g_gravity_vec * cl->GetInverseMass());
        }
      }
    }
  }

  void Graviton::PreRender(const float& dt) {}

  void Graviton::Render(const float& dt) {}

  void Graviton::PostRender(const float& dt) {}

  void Graviton::Initialize() {}
}
