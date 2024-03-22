#include "pch.h"
#include "clCubifyScript.h"

#include "clFezPlayerScript.h"
#include "egObject.hpp"
#include "egObjectBase.hpp"
#include "egTransform.h"
#include "egBaseCollider.hpp"
#include "egCamera.h"
#include "egGlobal.h"
#include "egProjectionFrustum.h"
#include "egRigidbody.h"

SERIALIZE_IMPL
(
 Client::Scripts::CubifyScript,
 _ARTAG(_BSTSUPER(Engine::Script))
 _ARTAG(m_cube_ids_)
 _ARTAG(m_z_length_)
 _ARTAG(m_x_length_)
)

namespace Client::Scripts
{
  CubifyScript::CubifyScript(const WeakObjectBase& owner)
    : Script(SCRIPT_T_CUBIFY, owner),
      m_z_length_(0),
      m_x_length_(0) {}

  CubifyScript::~CubifyScript() = default;

  void CubifyScript::Initialize()
  {
    Script::Initialize();

    UpdateCubes();
  }

  void CubifyScript::PreUpdate(const float& dt) {}

  void CubifyScript::Update(const float& dt)
  {
    StrongScene scene;
    StrongObjectBase owner;
    StrongObjectBase player;

    if (!LockWeak(GetOwner(), owner)) { return; }
    if (!LockWeak(owner->GetScene(), scene)) { return; }
    if (!LockWeak(scene->GetMainActor(), player)) { return; }

    const auto& player_script = player->GetScript<FezPlayerScript>().lock();
    if (!player_script) { return; }

    const auto& state = player_script->GetState();
    const auto& prev_state = player_script->GetPrevState();

    if (prev_state == CHAR_STATE_ROTATE && state == CHAR_STATE_POST_ROTATE)
    {
      UpdateCubes();
    }
  }

  void CubifyScript::PostUpdate(const float& dt) {}

  void CubifyScript::FixedUpdate(const float& dt) {}

  void CubifyScript::PreRender(const float& dt) {}

  void CubifyScript::Render(const float& dt) {}

  void CubifyScript::PostRender(const float& dt) {}

  WeakObjectBase CubifyScript::GetDepthNearestCube(const Vector3& pos) const
  {
    const auto& owner = GetOwner().lock();

    if (!owner) { return {}; }

    WeakObjectBase nearest_cube;
    float nearest_distance = std::numeric_limits<float>::max();

    for (const auto& id : m_cube_ids_)
    {
      if (const auto& cube = owner->GetChild(id).lock())
      {
        const auto& tr = cube->GetComponent<Components::Transform>().lock();

        if (!tr) { continue; }
        // If cube is not active, then cube is not in the view.
        if (!cube->GetActive()) { continue; }

        // Blocking player teleporting to upper side.
        if (tr->GetWorldPosition().y > pos.y) { continue; }

        const auto& distance = Vector3::DistanceSquared(tr->GetWorldPosition(), pos);

        if (distance < nearest_distance)
        {
          nearest_distance = distance;
          nearest_cube     = cube;
        }
      }
    }

    return nearest_cube;
  }

  void CubifyScript::OnCollisionEnter(const WeakCollider& other) {}

  void CubifyScript::OnCollisionContinue(const WeakCollider& other) {}

  void CubifyScript::OnCollisionExit(const WeakCollider& other) {}

  SCRIPT_CLONE_IMPL(CubifyScript)

  CubifyScript::CubifyScript()
    : Script(SCRIPT_T_CUBIFY, {}),
      m_z_length_(0),
      m_x_length_(0) {}

  void CubifyScript::UpdateCubes()
  {
     // Cube statics
    constexpr auto x_step = s_cube_dimension.x;
    constexpr auto y_step = s_cube_dimension.y;
    constexpr auto z_step = s_cube_dimension.z;

    constexpr auto x_step_half = x_step * 0.5f;
    constexpr auto y_step_half = y_step * 0.5f;
    constexpr auto z_step_half = z_step * 0.5f;

    constexpr Vector3 move_offsets[4] =
    {
      {1.f, 0.f, 0.f},
      {0.f, 0.f, 1.f},
      {-1.f, 0.f, 0.f},
      {0.f, 0.f, -1.f}
    };

    constexpr float move_steps[4] =
    {
      x_step,
      z_step,
      x_step,
      z_step
    };

    if (const auto& owner = GetOwner().lock())
    {
      StrongScene scene;
      if (!LockWeak(owner->GetScene(), scene)) { return; }

      for (const auto& id : m_cube_ids_)
      {
        if (const auto& cube = owner->GetChild(id).lock())
        {
          scene->RemoveGameObject(cube->GetID(), cube->GetLayer());
        }
      }

      m_cube_ids_.clear();

      StrongCamera camera;
      if (!LockWeak(scene->GetMainCamera(), camera)) { return; }

      const auto& cldr = owner->GetComponent<Components::Collider>().lock();
      const auto& rb = owner->GetComponent<Components::Rigidbody>().lock();
      const auto& tr = owner->GetComponent<Components::Transform>().lock();
      if (!cldr || !rb || !tr) { return; }

      const auto& scale = tr->GetLocalScale();
      const auto& half_scale = scale * 0.5f;
      const auto& cam_forward = camera->GetComponent<Components::Transform>().lock()->Forward();

      int offset = 0;

      if (Vector3Compare(cam_forward, g_forward)) { offset = 0; }
      else if (Vector3Compare(cam_forward, Vector3::Left)) { offset = 1; }
      else if (Vector3Compare(cam_forward, g_backward)) { offset = 2; }
      else if (Vector3Compare(cam_forward, Vector3::Right)) { offset = 3; }

      const Vector3 offsets[4] =
      {
        move_offsets[0] * x_step,
        move_offsets[1] * z_step,
        move_offsets[2] * x_step,
        move_offsets[3] * z_step
      };

      // todo: refactoring
      const Vector3 start_poses[4] = 
      {
        {-half_scale.x + x_step_half, -half_scale.y + x_step_half, -half_scale.z + z_step_half},
        {half_scale.x - x_step_half, -half_scale.y + x_step_half, -half_scale.z + x_step_half},
        {half_scale.x - x_step_half, -half_scale.y + x_step_half, half_scale.z - z_step_half},
        {-half_scale.x + x_step_half, -half_scale.y + x_step_half, half_scale.z - z_step_half}
      };

      const float x_count = half_scale.x / x_step;
      const float z_count = half_scale.z / z_step;

      m_z_length_ = static_cast<int>(z_count / 0.5f);
      m_x_length_ = static_cast<int>(x_count / 0.5f);

      Vector3 start_pos = start_poses[offset];

      const int target_count = offset == 0 || offset == 2 ? m_z_length_ : m_x_length_;

      for (float y = start_pos.y; y < half_scale.y; y += y_step) // NOLINT(cert-flp30-c)
      {
        for (int i = 0; i < target_count; ++i)
        {
          const auto& cube = scene->CreateGameObject<Object>(LAYER_ENVIRONMENT).lock();
          cube->SetName(owner->GetName() + " Cube " + std::format("{}_{}_{}", start_pos.x, y, start_pos.z));
          const auto& cube_tr = cube->AddComponent<Components::Transform>().lock();

          cube_tr->SetLocalPosition(start_pos);

          cube->AddComponent<Components::Collider>();
          const auto& crb = cube->AddComponent<Components::Rigidbody>().lock();
          crb->SetFixed(true);
          crb->SetGravityOverride(false);

          owner->AddChild(cube);

          m_cube_ids_.push_back(cube->GetLocalID());

          start_pos += offsets[offset];
        }
      }
    }
  }
}
