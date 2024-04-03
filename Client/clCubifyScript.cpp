#include "pch.h"
#include "clCubifyScript.h"

#include "clFezPlayerScript.h"
#include "egObject.hpp"
#include "egObjectBase.hpp"
#include "egTransform.h"
#include "egBaseCollider.hpp"
#include "egCamera.h"
#include "egCollisionDetector.h"
#include "egGlobal.h"
#include "egImGuiHeler.hpp"
#include "egProjectionFrustum.h"
#include "egRigidbody.h"
#include "egSceneManager.hpp"

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
      m_cube_dimension_(Vector3::One),
      m_cube_type_(CUBE_TYPE_NORMAL),
      m_z_length_(0),
      m_y_length_(0),
      m_x_length_(0) {}

  CubifyScript::~CubifyScript() = default;

  void CubifyScript::Initialize()
  {
    Script::Initialize();
  }

  void CubifyScript::PreUpdate(const float& dt) { }

  void CubifyScript::Update(const float& dt) { }

  void CubifyScript::UpdateCubes(bool normal)
  {
    StrongScene      scene;
    StrongObjectBase owner;
    StrongObjectBase player;

    if (!LockWeak(GetOwner(), owner)) { return; }
    if (!LockWeak(owner->GetScene(), scene)) { return; }
    if (!LockWeak(scene->GetMainActor(), player)) { return; }

    const auto& player_script = player->GetScript<FezPlayerScript>().lock();
    if (!player_script) { return; }

    updateCubesImpl(normal);
  }

  void CubifyScript::PostUpdate(const float& dt) { }

  void CubifyScript::FixedUpdate(const float& dt) {}

  void CubifyScript::PreRender(const float& dt) {}

  void CubifyScript::Render(const float& dt) {}

  void CubifyScript::PostRender(const float& dt) {}

  void CubifyScript::OnImGui()
  {
    Script::OnImGui();

    if (ImGuiVector3Editable("Cube Dimension", GetID(), "cube_dimension", m_cube_dimension_, 0.1f, 0.1f))
    {
      CubifyScript::DispatchNormalUpdate();
      CubifyScript::DispatchUpdateWithoutNormal();
    }

    if (ImGui::Combo("Cube Type", reinterpret_cast<int*>(&m_cube_type_), "Normal\0Ladder\0Ice\0"))
    {
      CubifyScript::DispatchNormalUpdate();
      CubifyScript::DispatchUpdateWithoutNormal();
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

  void CubifyScript::DispatchNormalUpdate()
  {
    StrongScene scene;
    if (!LockWeak(GetSceneManager().GetActiveScene(), scene)) { return; }

    const auto& scripts = scene->GetCachedScripts<CubifyScript>();

    for (const auto& scp : scripts)
    {
      if (const auto& locked = scp.lock())
      {
        const auto& casted = boost::static_pointer_cast<CubifyScript>(locked);
        if (casted->GetCubeType() == CUBE_TYPE_NORMAL)
        {
          casted->UpdateCubes(true);
        }
      }
    }
  }

  void CubifyScript::DispatchUpdateWithoutNormal()
  {
    StrongScene scene;
    if (!LockWeak(GetSceneManager().GetActiveScene(), scene)) { return; }

    const auto& scripts = scene->GetCachedScripts<CubifyScript>();

    for (const auto& scp : scripts)
    {
      if (const auto& locked = scp.lock())
      {
        const auto& casted = boost::static_pointer_cast<CubifyScript>(locked);
        if (casted->GetCubeType() != CUBE_TYPE_NORMAL)
        {
          casted->UpdateCubes(false);
        }
      }
    }
  }

  void CubifyScript::OnCollisionEnter(const WeakCollider& other) {}

  void CubifyScript::OnCollisionContinue(const WeakCollider& other) {}

  void CubifyScript::OnCollisionExit(const WeakCollider& other) {}

  SCRIPT_CLONE_IMPL(CubifyScript)

  CubifyScript::CubifyScript()
    : Script(SCRIPT_T_CUBIFY, {}),
      m_cube_dimension_(Vector3::One),
      m_cube_type_(CUBE_TYPE_NORMAL),
      m_z_length_(0),
      m_y_length_(0),
      m_x_length_(0) {}

  void CubifyScript::updateCubesImpl(bool normal)
  {
    ZeroToEpsilon(m_cube_dimension_);

     // Cube statics
    const auto x_step = m_cube_dimension_.x;
    const auto y_step = m_cube_dimension_.y;
    const auto z_step = m_cube_dimension_.z;

    const auto x_step_half = x_step * 0.5f;
    const auto y_step_half = y_step * 0.5f;
    const auto z_step_half = z_step * 0.5f;

    const float move_step_by_rotation[4] =
    {
      x_step,
      z_step,
      x_step,
      z_step
    };

    const Vector3 move_offset_by_rotation[4] =
    {
      s_move_offsets[0] * x_step,
      s_move_offsets[1] * z_step,
      s_move_offsets[2] * x_step,
      s_move_offsets[3] * z_step
    };

    // Fix values of axis with player's value, to correct the cube with player's movement.
    // This values are same as the camera's forward, left, backward, right.
    static const Vector3 fixed_axis[4] =
    {
      g_forward,
      Vector3::Left,
      g_backward,
      Vector3::Right
    };

    if (const auto& owner = GetOwner().lock())
    {
      StrongObjectBase player;
      StrongScene      scene;
      if (!LockWeak(owner->GetScene(), scene)) { return; }
      if (!LockWeak(scene->GetMainActor(), player)) { return; }

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
      const auto& tr = owner->GetComponent<Components::Transform>().lock();
      if (!cldr || !tr) { return; }

      const auto& scale = tr->GetLocalScale();
      const auto& half_scale = scale * 0.5f;
      const auto& cam_forward = camera->GetComponent<Components::Transform>().lock()->Forward();
      const auto& player_tr = player->GetComponent<Components::Transform>().lock();
      const auto& player_script = player->GetScript<FezPlayerScript>().lock();
      const auto& player_pos = player_tr->GetWorldPosition();
      const auto& obj_pos = tr->GetWorldPosition();

      const int rotation_offset = player_script->GetRotationOffset();

      const float x_count = half_scale.x / x_step;
      const float y_count = half_scale.y / y_step;
      const float z_count = half_scale.z / z_step;

      m_z_length_ = static_cast<int>(z_count * 2.f);
      m_y_length_ = static_cast<int>(y_count * 2.f);
      m_x_length_ = static_cast<int>(x_count * 2.f);

      // todo: refactoring
      const Vector3 start_pos_by_rotation[4] = 
      {
        {-half_scale.x + x_step_half, -half_scale.y + y_step_half, -half_scale.z + z_step_half},
        {half_scale.x - x_step_half, -half_scale.y + y_step_half, -half_scale.z + z_step_half},
        {half_scale.x - x_step_half, -half_scale.y + y_step_half, half_scale.z - z_step_half},
        {-half_scale.x + x_step_half, -half_scale.y + y_step_half, half_scale.z - z_step_half}
      };

      Vector3 start_pos = start_pos_by_rotation[rotation_offset];

      if (!normal && m_cube_type_ != CUBE_TYPE_NORMAL)
      {
        const auto& delta = player_pos - obj_pos;
        const auto& axis_offset = fixed_axis[rotation_offset] * delta.Dot(fixed_axis[rotation_offset]);
        start_pos = start_pos + axis_offset;
      }

      float y = start_pos.y;

      const int target_count = rotation_offset == 0 || rotation_offset == 3 ? m_x_length_ : m_z_length_;

      for (int i = 0; i < m_y_length_; ++i)
      {
        start_pos = start_pos_by_rotation[rotation_offset];

        if (!normal && m_cube_type_ != CUBE_TYPE_NORMAL)
        {
          const auto& delta = player_pos - obj_pos;
          const auto& proj = delta.Dot(fixed_axis[rotation_offset]);
          const auto& axis_offset = fixed_axis[rotation_offset] * proj;
          start_pos = start_pos + axis_offset;
        }

        start_pos.y = y;

        for (int j = 0; j < target_count; ++j)
        {
          const auto& cube = scene->CreateGameObject<Object>(LAYER_ENVIRONMENT).lock();
          cube->SetName(owner->GetName() + " Cube " + std::format("{}_{}_{}", start_pos.x, start_pos.y, start_pos.z));
          const auto& cube_tr = cube->AddComponent<Components::Transform>().lock();

          cube->AddComponent<Components::Collider>();

          owner->AddChild(cube, true);

          cube_tr->SetLocalScale(m_cube_dimension_);
          // Note that this uses local space.
          cube_tr->SetLocalPosition(start_pos);

          m_cube_ids_.push_back(cube->GetLocalID());

          start_pos += move_offset_by_rotation[rotation_offset];
        }

        y += y_step;
      }
    }
  }
}
