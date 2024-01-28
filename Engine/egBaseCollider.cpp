#include "pch.h"
#include "egBaseCollider.hpp"
#include <imgui_stdlib.h>
#include "egCollision.h"
#include "egCubeMesh.h"
#include "egD3Device.hpp"
#include "egGlobal.h"
#include "egResourceManager.hpp"
#include "egSceneManager.hpp"
#include "egShape.h"
#include "egSphereMesh.h"
#include "egTransform.h"

#include "egManagerHelper.hpp"

SERIALIZER_ACCESS_IMPL
(
 Engine::Components::Collider,
 _ARTAG(_BSTSUPER(Engine::Abstract::Component))
 _ARTAG(m_type_) _ARTAG(m_model_name_) _ARTAG(m_mass_)
 _ARTAG(m_boundings_)
)

namespace Engine::Components
{
  const std::vector<Graphics::VertexElement>& Collider::GetVertices() const
  {
    if (const auto model = m_model_.lock()) { return model->GetVertices(); }

    if (m_type_ == BOUNDING_TYPE_BOX) { return m_cube_stock_; }
    if (m_type_ == BOUNDING_TYPE_SPHERE) { return m_sphere_stock_; }

    return {};
  }

  Matrix Collider::GetWorldMatrix() const
  {
    return GetLocalMatrix() * GetOwner().lock()->GetComponent<Transform>().lock()->GetWorldMatrix();
  }

  Matrix Collider::GetLocalMatrix() const { return m_local_matrix_; }

  void Collider::Initialize()
  {
    Component::Initialize();

    InitializeStockVertices();

    const auto vtx_ptr = reinterpret_cast<Vector3*>(m_cube_stock_.data());
    m_boundings_.CreateFromPoints<BoundingBox>(m_cube_stock_.size(), vtx_ptr, sizeof(Graphics::VertexElement));

    if (m_type_ == BOUNDING_TYPE_BOX) { GenerateInertiaCube(); }
    else if (m_type_ == BOUNDING_TYPE_SPHERE) { GenerateInertiaSphere(); }

    UpdateInertiaTensor();
  }

  void Collider::InitializeStockVertices()
  {
    GeometricPrimitive::IndexCollection  index;
    GeometricPrimitive::VertexCollection vertex;

    if (m_cube_stock_.empty())
    {
      GeometricPrimitive::CreateCube(vertex, index, 1.f, false);

      for (const auto& v : vertex) { m_cube_stock_.push_back({v.position}); }
    }
    if (m_sphere_stock_.empty())
    {
      GeometricPrimitive::CreateSphere(vertex, index, 1.f, 16, false);

      for (const auto& v : vertex) { m_sphere_stock_.push_back({v.position}); }
    }
  }

  void Collider::FromMatrix(const Matrix& mat) { m_local_matrix_ = mat; }

  void Collider::SetType(const eBoundingType type)
  {
    m_type_ = type;
    m_boundings_.SetType(type);

    if (m_type_ == BOUNDING_TYPE_BOX) { GenerateInertiaCube(); }
    else if (m_type_ == BOUNDING_TYPE_SPHERE) { GenerateInertiaSphere(); }

    UpdateInertiaTensor();
  }

  void Collider::SetMass(const float mass) { m_mass_ = mass; }

  void Collider::SetBoundingBox(const BoundingOrientedBox& bounding)
  {
    m_boundings_.UpdateFromBoundingBox(bounding);

    if (m_type_ == BOUNDING_TYPE_BOX) { GenerateInertiaCube(); }
    else if (m_type_ == BOUNDING_TYPE_SPHERE) { GenerateInertiaSphere(); }

    UpdateInertiaTensor();
  }

  void Collider::SetModel(const WeakModel& model)
  {
    if (const auto locked = model.lock())
    {
      m_model_name_ = locked->GetName();
      m_model_      = locked;

      BoundingOrientedBox obb;
      BoundingOrientedBox::CreateFromBoundingBox(obb, locked->GetBoundingBox());
      SetBoundingBox(obb);
    }
  }

  bool Collider::Intersects(const StrongCollider& lhs, const StrongCollider& rhs, const Vector3& dir)
  {
    return lhs->m_boundings_.Intersects(rhs->m_boundings_, lhs->GetWorldMatrix(), rhs->GetWorldMatrix(), dir);
  }

  bool Collider::Intersects(const StrongCollider& lhs, const StrongCollider& rhs, const float epsilon)
  {
    return lhs->m_boundings_.Intersects(rhs->m_boundings_, lhs->GetWorldMatrix(), rhs->GetWorldMatrix(), epsilon);
  }

  bool Collider::Intersects(
    const StrongCollider& lhs, const StrongCollider& rhs, const float dist, const Vector3& dir
  )
  {
    float      distance = 0.f;
    const auto test     = lhs->GetBounding().TestRay(rhs->GetBounding(), dir, distance);
    return test && distance <= dist;
  }

  bool Collider::Intersects(
    const Vector3& start, const Vector3& dir, float distance,
    float&         intersection
  ) const
  {
    if (m_type_ == BOUNDING_TYPE_BOX)
    {
      const auto    box     = GetBounding<BoundingOrientedBox>();
      const Vector3 Extents = box.Extents;
      const auto    test    = Physics::Raycast::TestRayOBBIntersection
        (
         start, dir, -Extents,
         Extents,
         GetWorldMatrix(),
         intersection
        );

      return test && intersection <= distance;
    }
    if (m_type_ == BOUNDING_TYPE_SPHERE)
    {
      const auto sphere = GetBounding<BoundingSphere>();
      const auto test   = Physics::Raycast::TestRaySphereIntersection
        (
         start, dir, sphere.Center,
         sphere.Radius,
         intersection
        );

      return test && intersection <= distance;
    }

    return false;
  }

  void Collider::AddCollidedObject(const GlobalEntityID id)
  {
    m_collided_objects_.insert(id);

    if (!m_cps_.contains(id)) { m_cps_.insert({id, 1}); }
    else { m_cps_[id]++; }

    if (!m_ltcc_.contains(id)) { m_ltcc_.insert({id, 1}); }
    else { m_ltcc_[id]++; }
  }

  void Collider::RemoveCollidedObject(const GlobalEntityID id)
  {
    m_collided_objects_.erase(id);
    if (m_cps_.contains(id)) { m_cps_.erase(id); }
  }

  bool Collider::IsCollidedObject(const GlobalEntityID id) const { return m_collided_objects_.contains(id); }

  const std::set<GlobalEntityID>& Collider::GetCollidedObjects() const { return m_collided_objects_; }

  UINT Collider::GetCPS(GlobalEntityID id) const
  {
    if (!m_cps_.contains(id)) { return 0; }

    return m_cps_.at(id);
  }

  bool Collider::GetPenetration(
    const Collider& other, Vector3& normal,
    float&          depth
  ) const
  {
    auto dir = other.GetWorldMatrix().Translation() - GetWorldMatrix().Translation();
    dir.Normalize();

    return Physics::GJK::GJKAlgorithm
      (
       GetWorldMatrix(), other.GetWorldMatrix(), GetVertices(), other.GetVertices(), dir,
       normal, depth
      );
  }

  UINT Collider::GetCollisionCount(const GlobalEntityID id) const
  {
    if (!m_ltcc_.contains(id)) { return 0; }

    return m_ltcc_.at(id);
  }

  float Collider::GetMass() const { return m_mass_; }

  float Collider::GetInverseMass() const { return 1.0f / m_mass_; }

  XMFLOAT3X3 Collider::GetInertiaTensor() const { return m_inertia_tensor_; }

  eBoundingType Collider::GetType() const { return m_type_; }

  Collider::Collider()
    : Component(COM_T_COLLIDER, {}),
      m_fps_counter_(0),
      m_ltcc_counter_(0),
      m_type_(BOUNDING_TYPE_BOX),
      m_boundings_(),
      m_mass_(1.f),
      m_inertia_tensor_(),
      m_local_matrix_(Matrix::Identity) {}

  void Collider::FixedUpdate(const float& dt) {}

  void Collider::OnDeserialized()
  {
    Component::OnDeserialized();

    InitializeStockVertices();

    if (m_type_ == BOUNDING_TYPE_BOX) { GenerateInertiaCube(); }
    else if (m_type_ == BOUNDING_TYPE_SPHERE) { GenerateInertiaSphere(); }

    UpdateInertiaTensor();
  }

  void Collider::OnImGui()
  {
    Component::OnImGui();
    ImGui::Indent(2);

    ImGui::InputInt("Collider Type", reinterpret_cast<int*>(&m_type_));

    ImGui::InputFloat("Collider Mass", &m_mass_);

    // TODO: colliding objects
  }

  Physics::GenericBounding Collider::GetBounding() const { return m_boundings_.Transform(GetWorldMatrix()); }

  void Collider::UpdateInertiaTensor()
  {
    Quaternion rotation;

    if (m_type_ == BOUNDING_TYPE_BOX) { rotation = GetBounding<BoundingOrientedBox>().Orientation; }
    else if (m_type_ == BOUNDING_TYPE_SPHERE) { rotation = Quaternion::Identity; }

    Quaternion conjugate;

    rotation.Conjugate(conjugate);
    const Matrix invOrientation = Matrix::CreateFromQuaternion(conjugate);
    const Matrix orientation    = Matrix::CreateFromQuaternion(rotation);

    Vector3 dimensions;

    if (m_type_ == BOUNDING_TYPE_BOX) { dimensions = Vector3(GetBounding<BoundingOrientedBox>().Extents) * 2; }
    else if (m_type_ == BOUNDING_TYPE_SPHERE) { dimensions = Vector3(GetBounding<BoundingSphere>().Radius) * 2; }

    const Matrix matrix = orientation *
                          Matrix::CreateScale(dimensions) * invOrientation;

    XMStoreFloat3x3(&m_inertia_tensor_, matrix);
  }

  Collider::Collider(const WeakObject& owner)
    : Component(COM_T_COLLIDER, owner),
      m_fps_counter_(0),
      m_ltcc_counter_(0),
      m_type_(BOUNDING_TYPE_BOX),
      m_boundings_(),
      m_mass_(1.0f),
      m_inertia_tensor_(),
      m_local_matrix_(Matrix::Identity) { }

  void Collider::GenerateInertiaCube()
  {
    const Vector3 dim                = GetBounding<BoundingOrientedBox>().Extents;
    const Vector3 dimensions_squared = dim * dim;

    m_inverse_inertia_.x = (12.0f * GetInverseMass()) /
                           (dimensions_squared.y + dimensions_squared.z);
    m_inverse_inertia_.y = (12.0f * GetInverseMass()) /
                           (dimensions_squared.x + dimensions_squared.z);
    m_inverse_inertia_.z = (12.0f * GetInverseMass()) /
                           (dimensions_squared.x + dimensions_squared.y);
  }

  void Collider::GenerateInertiaSphere()
  {
    const float radius = GetBounding<BoundingSphere>().Radius;
    const float i      = 2.5f * GetInverseMass() / (radius * radius);

    m_inverse_inertia_ = Vector3(i, i, i);
  }

  void Collider::PreUpdate(const float& dt)
  {
    if (m_fps_counter_ >= 1.f)
    {
      for (const auto& [id, count] : m_cps_)
      {
        if (m_ltcc_.contains(id)) { m_ltcc_[id] += m_cps_[id]; }
        else { m_ltcc_.insert({id, count}); }
      }

      m_cps_.clear();

      for (auto it = m_ltcc_.begin(); it != m_ltcc_.end();)
      {
        if (it->second == 0) { it = m_ltcc_.erase(it); }
        else if (it->second > g_energy_reduction_ceil)
        {
          it->second = g_energy_reduction_ceil;
          ++it;
        }
        else { ++it; }
      }

      m_fps_counter_ = 0.f;
    }

    if (m_ltcc_counter_ >= g_ltcc_window_interval)
    {
      for (auto it = m_ltcc_.begin(); it != m_ltcc_.end();)
      {
        if (it->second > 0)
        {
          if (!m_collided_objects_.contains(it->first)) { it->second--; }
          if (it->second > g_energy_reduction_ceil) { it->second = g_energy_reduction_ceil; }

          ++it;
        }
        else { it = m_ltcc_.erase(it); }
      }

      m_ltcc_counter_ = 0.f;
    }

    m_fps_counter_ += dt;
    m_ltcc_counter_ += dt;

    UpdateInertiaTensor();
  }

  void Collider::Update(const float& dt) { UpdateInertiaTensor(); }

  void Collider::PostUpdate(const float& dt)
  {
    Component::PostUpdate(dt);
#ifdef _DEBUG
    if (m_collided_objects_.empty())
    {
      if (m_type_ == BOUNDING_TYPE_BOX) { GetDebugger().Draw(GetBounding<BoundingOrientedBox>(), Colors::OrangeRed); }
      else { GetDebugger().Draw(GetBounding<BoundingSphere>(), Colors::OrangeRed); }
    }
    else
    {
      if (m_type_ == BOUNDING_TYPE_BOX) { GetDebugger().Draw(GetBounding<BoundingOrientedBox>(), Colors::GreenYellow); }
      else { GetDebugger().Draw(GetBounding<BoundingSphere>(), Colors::GreenYellow); }
    }
#endif
  }
} // namespace Engine::Component
