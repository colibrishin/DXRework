#include "pch.h"
#include "clCubifyScript.h"

#include "clFezPlayerScript.h"
#include "egObject.hpp"
#include "egObjectBase.hpp"
#include "egTransform.h"
#include "egBaseCollider.hpp"
#include "egCamera.h"
#include "egGlobal.h"
#include "egImGuiHeler.hpp"
#include "egProjectionFrustum.h"
#include "egRigidbody.h"

SERIALIZE_IMPL
(
 Client::Scripts::CubifyScript,
 _ARTAG(_BSTSUPER(Engine::Script))
 _ARTAG(m_cube_ids_)
 _ARTAG(m_cube_dimension_)
 _ARTAG(m_z_length_)
 _ARTAG(m_x_length_)
 _ARTAG(m_cube_type_)
)

namespace Client::Scripts
{
  CubifyScript::CubifyScript(const WeakObjectBase& owner)
    : Script(SCRIPT_T_CUBIFY, owner),
      m_z_length_(0),
      m_x_length_(0),
      m_cube_dimension_(Vector3::One),
      m_cube_type_(CUBE_TYPE_NORMAL) {}

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

  void CubifyScript::OnImGui()
  {
    Script::OnImGui();

    if (ImGuiVector3Editable("Cube Dimension", GetID(), "cube_dimension", m_cube_dimension_, 0.1f, 0.1f))
    {
      UpdateCubes();
    }

    if (ImGui::Combo("Cube Type", reinterpret_cast<int*>(&m_cube_type_), "Normal\0Ladder\0Ice\0"))
    {
      UpdateCubes();
    }
  }

  void CubifyScript::SetCubeDimension(const Vector3& dimension)
  {
    if (dimension == Vector3::Zero) { return; }
    if (dimension.x <= 0 || dimension.y <= 0 || dimension.z <= 0) { return; }
    m_cube_dimension_ = dimension;
  }

  void CubifyScript::SetCubeType(const eCubeType type)
  {
    m_cube_type_ = type;
  }

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

  eCubeType CubifyScript::GetCubeType() const { return m_cube_type_; }

  void CubifyScript::OnCollisionEnter(const WeakCollider& other) {}

  void CubifyScript::OnCollisionContinue(const WeakCollider& other) {}

  void CubifyScript::OnCollisionExit(const WeakCollider& other) {}

  SCRIPT_CLONE_IMPL(CubifyScript)

  CubifyScript::CubifyScript()
    : Script(SCRIPT_T_CUBIFY, {}),
      m_z_length_(0),
      m_x_length_(0),
      m_cube_dimension_(Vector3::One),
      m_cube_type_(CUBE_TYPE_NORMAL) {}

  void CubifyScript::UpdateCubes()
  {
    if (m_cube_dimension_.x == 0.f)
    {
      m_cube_dimension_.x = g_epsilon;
    }
    if (m_cube_dimension_.y == 0.f)
    {
      m_cube_dimension_.y = g_epsilon;
    }
    if (m_cube_dimension_.z == 0.f)
    {
      m_cube_dimension_.z = g_epsilon;
    }

     // Cube statics
    const auto x_step = m_cube_dimension_.x;
    const auto y_step = m_cube_dimension_.y;
    const auto z_step = m_cube_dimension_.z;

    const auto x_step_half = x_step * 0.5f;
    const auto y_step_half = y_step * 0.5f;
    const auto z_step_half = z_step * 0.5f;

    constexpr Vector3 move_offsets[4] =
    {
      {1.f, 0.f, 0.f},
      {0.f, 0.f, 1.f},
      {-1.f, 0.f, 0.f},
      {0.f, 0.f, -1.f}
    };

    const float move_steps[4] =
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
        {-half_scale.x + x_step_half, -half_scale.y + y_step_half, -half_scale.z + z_step_half},
        {half_scale.x - x_step_half, -half_scale.y + y_step_half, -half_scale.z + z_step_half},
        {half_scale.x - x_step_half, -half_scale.y + y_step_half, half_scale.z - z_step_half},
        {-half_scale.x + x_step_half, -half_scale.y + y_step_half, half_scale.z - z_step_half}
      };

      const float x_count = half_scale.x / x_step;
      const float z_count = half_scale.z / z_step;

      m_z_length_ = static_cast<int>(z_count * 2.f);
      m_x_length_ = static_cast<int>(x_count * 2.f);

      Vector3 start_pos = start_poses[offset];

      const int target_count = offset == 0 || offset == 3 ? m_x_length_ : m_z_length_;

      for (float y = start_pos.y; y < half_scale.y; y += y_step) // NOLINT(cert-flp30-c)
      {
        start_pos = start_poses[offset];
        start_pos.y = y;

        for (int i = 0; i < target_count; ++i)
        {
          const auto& cube = scene->CreateGameObject<Object>(LAYER_ENVIRONMENT).lock();
          cube->SetName(owner->GetName() + " Cube " + std::format("{}_{}_{}", start_pos.x, y, start_pos.z));
          const auto& cube_tr = cube->AddComponent<Components::Transform>().lock();

          cube_tr->SetLocalScale(m_cube_dimension_);
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
