#include "pch.h"
#include "clCubifyScript.h"

#include "egObject.hpp"
#include "egObjectBase.hpp"
#include "egTransform.h"
#include "egBaseCollider.hpp"
#include "egProjectionFrustum.h"
#include "egRigidbody.h"

SERIALIZE_IMPL
(
 Client::Scripts::CubifyScript,
 _ARTAG(_BSTSUPER(Engine::Script))
)

namespace Client::Scripts
{
  CubifyScript::CubifyScript(const WeakObjectBase& owner)
    : Script(SCRIPT_T_CUBIFY, owner) {}

  CubifyScript::~CubifyScript() = default;

  void CubifyScript::Initialize()
  {
    Script::Initialize();

    if (const auto& owner = GetOwner().lock())
    {
      const auto& cldr = owner->GetComponent<Components::Collider>().lock();
      const auto& rb = owner->GetComponent<Components::Rigidbody>().lock();

      if (!cldr || !rb) { return; }
      cldr->SetActive(false);
      rb->SetActive(false);

      if (const auto& tr = owner->GetComponent<Components::Transform>().lock())
      {
        const auto& scale = tr->GetLocalScale();
        const auto& scene = owner->GetScene().lock();

        if (!scene) { return; }

        const auto& half_scale = scale * 0.5f;

        // Split object into cubes.
        constexpr auto x_step = s_cube_dimension.x;
        constexpr auto y_step = s_cube_dimension.y;
        constexpr auto z_step = s_cube_dimension.z;

        constexpr auto x_step_half = x_step * 0.5f;
        constexpr auto y_step_half = s_cube_dimension.y * 0.5f;
        constexpr auto z_step_half = z_step * 0.5f;

        for (float x = -half_scale.x + x_step_half; x < half_scale.x; x += x_step)  // NOLINT(cert-flp30-c)
        {
          for (float y = -half_scale.y + y_step_half; y < half_scale.y; y += y_step) // NOLINT(cert-flp30-c)
          {
            for (float z = -half_scale.z + z_step_half; z < half_scale.z; z += z_step) // NOLINT(cert-flp30-c)
            {
              const auto& cube = scene->CreateGameObject<Object>(LAYER_ENVIRONMENT).lock();
              cube->SetName(owner->GetName() + " Cube " + std::format("{}_{}_{}", x, 0.f, z));
              const auto& cube_tr = cube->AddComponent<Components::Transform>().lock();

              cube_tr->SetLocalPosition({x, 0.f, z});

              cube->AddComponent<Components::Collider>();
              const auto& crb = cube->AddComponent<Components::Rigidbody>().lock();
              crb->SetFixed(true);
              crb->SetGravityOverride(false);

              owner->AddChild(cube);

              m_cube_ids_.push_back(cube->GetLocalID());
            }
          }
        }
      }
    }
  }

  void CubifyScript::PreUpdate(const float& dt) {}

  void CubifyScript::Update(const float& dt)
  {
    const auto& owner = GetOwner().lock();

    if (!owner) { return; }

    // Active cubes only if they are in the frustum (in the camera view)
    for (const auto& id : m_cube_ids_)
    {
      if (const auto& cube = owner->GetChild(id).lock())
      {
        if (!GetProjectionFrustum().CheckRender(cube))
        {
          cube->SetActive(false);
        }
        else
        {
          cube->SetActive(true);
        }
      }
    }
  }

  void CubifyScript::PostUpdate(const float& dt) {}

  void CubifyScript::FixedUpdate(const float& dt) {}

  void CubifyScript::PreRender(const float& dt) {}

  void CubifyScript::Render(const float& dt) {}

  void CubifyScript::PostRender(const float& dt) {}

  void CubifyScript::OnCollisionEnter(const WeakCollider& other) {}

  void CubifyScript::OnCollisionContinue(const WeakCollider& other) {}

  void CubifyScript::OnCollisionExit(const WeakCollider& other) {}

  SCRIPT_CLONE_IMPL(CubifyScript)

  CubifyScript::CubifyScript() : Script(SCRIPT_T_CUBIFY, {}) {}
}
