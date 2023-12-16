#include "pch.h"
#include "egCollider.hpp"
#include <imgui_stdlib.h>
#include "egCollision.h"
#include "egCubeMesh.h"
#include "egD3Device.hpp"
#include "egResourceManager.h"
#include "egSceneManager.hpp"
#include "egSphereMesh.h"
#include "egTransform.h"

#include "egManagerHelper.hpp"

SERIALIZER_ACCESS_IMPL(
                       Engine::Component::Collider,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Component))
                       _ARTAG(m_bDirtyByTransform) _ARTAG(m_position_)
                       _ARTAG(m_rotation_) _ARTAG(m_size_)
                       _ARTAG(m_type_) _ARTAG(m_mass_) _ARTAG(m_offset_))

namespace Engine::Component
{
    void Collider::SetPosition(const Vector3& position)
    {
        m_position_ = position;

        if (m_type_ == BOUNDING_TYPE_BOX)
        {
            SetPosition_GENERAL_TYPE(m_boundings_.box, m_position_);
        }
        else if (m_type_ == BOUNDING_TYPE_SPHERE)
        {
            SetPosition_GENERAL_TYPE(m_boundings_.sphere, m_position_);
        }

        UpdateBoundings();
    }

    void Collider::SetRotation(const Quaternion& rotation)
    {
        m_rotation_ = rotation;

        if (m_type_ == BOUNDING_TYPE_BOX)
        {
            SetRotation_GENERAL_TYPE(m_boundings_.box, m_rotation_);
        }
        else if (m_type_ == BOUNDING_TYPE_SPHERE)
        {
            SetRotation_GENERAL_TYPE(m_boundings_.sphere, m_rotation_);
        }

        UpdateBoundings();
        UpdateInertiaTensor();
    }

    void Collider::SetYawPitchRoll(const Vector3& yaw_pitch_roll)
    {
        m_yaw_pitch_roll_degree_ = yaw_pitch_roll;
        m_rotation_              =
                Quaternion::CreateFromYawPitchRoll(
                                                   XMConvertToRadians(yaw_pitch_roll.x),
                                                   XMConvertToRadians(yaw_pitch_roll.y),
                                                   XMConvertToRadians(yaw_pitch_roll.z));

        if (m_type_ == BOUNDING_TYPE_BOX)
        {
            SetRotation_GENERAL_TYPE(m_boundings_.box, m_rotation_);
        }
        else if (m_type_ == BOUNDING_TYPE_SPHERE)
        {
            SetRotation_GENERAL_TYPE(m_boundings_.sphere, m_rotation_);
        }

        UpdateBoundings();
        UpdateInertiaTensor();
    }

    const std::vector<const Vector3*>& Collider::GetVertices() const
    {
        if (const auto mesh = m_mesh_.lock())
        {
            return mesh->GetVertices();
        }

        if (m_type_ == BOUNDING_TYPE_BOX)
        {
            return m_cube_stock_ref_;
        }
        if (m_type_ == BOUNDING_TYPE_SPHERE)
        {
            return m_sphere_stock_ref_;
        }

        return {};
    }

    void Collider::Initialize()
    {
        Component::Initialize();

        InitializeStockVertices();
        if (const auto mesh = m_mesh_.lock())
        {
            GenerateFromMesh(mesh);
        }

        if (m_type_ == BOUNDING_TYPE_BOX)
        {
            GenerateInertiaCube();
        }
        else if (m_type_ == BOUNDING_TYPE_SPHERE)
        {
            GenerateInertiaSphere();
        }

        UpdateFromTransform();
        UpdateInertiaTensor();
        UpdateBoundings();

        m_previous_position_ = m_position_;
    }

    void Collider::InitializeStockVertices()
    {
        GeometricPrimitive::IndexCollection  index;
        GeometricPrimitive::VertexCollection vertex;

        if (m_cube_stock_.empty())
        {
            GeometricPrimitive::CreateBox(vertex, index, Vector3::One, false);

            for (const auto& v : vertex)
            {
                m_cube_stock_.push_back(v.position);
            }

            for (const auto& v : m_cube_stock_)
            {
                m_cube_stock_ref_.push_back(&v);
            }
        }
        if (m_sphere_stock_.empty())
        {
            GeometricPrimitive::CreateSphere(vertex, index, 1.f, 16, false);

            for (const auto& v : vertex)
            {
                m_sphere_stock_.push_back(v.position);
            }

            for (const auto& v : m_sphere_stock_)
            {
                m_sphere_stock_ref_.push_back(&v);
            }
        }
    }

    inline void Collider::GenerateFromMesh(const WeakMesh& mesh)
    {
        const auto mesh_obj = mesh.lock();

        std::vector<Vector3> serialized_vertices;

        std::ranges::for_each(
                              mesh_obj->m_vertices_,
                              [&](const Resources::Shape& shape)
                              {
                                  for (int i = 0; i < shape.size(); ++i)
                                  {
                                      serialized_vertices.emplace_back(shape[i].position);
                                  }
                              });

        if (GetType() == BOUNDING_TYPE_BOX)
        {
            m_boundings_.CreateFromPoints<BoundingBox>(
                                                       serialized_vertices.size(),
                                                       serialized_vertices.data(),
                                                       sizeof(Vector3));
        }
        else if (GetType() == BOUNDING_TYPE_SPHERE)
        {
            m_boundings_.CreateFromPoints<BoundingSphere>(
                                                          serialized_vertices.size(),
                                                          serialized_vertices.data(),
                                                          sizeof(Vector3));
        }
    }

    void Collider::SetSize(const Vector3& size)
    {
        m_size_ = size;

        if (m_type_ == BOUNDING_TYPE_BOX)
        {
            GenerateInertiaCube();
        }
        else if (m_type_ == BOUNDING_TYPE_SPHERE)
        {
            GenerateInertiaSphere();
        }

        UpdateInertiaTensor();
    }

    void Collider::SetType(const eBoundingType type)
    {
        m_type_ = type;

        if (const auto mesh = m_mesh_.lock())
        {
            GenerateFromMesh(mesh);
        }

        if (m_type_ == BOUNDING_TYPE_BOX)
        {
            GenerateInertiaCube();
        }
        else if (m_type_ == BOUNDING_TYPE_SPHERE)
        {
            GenerateInertiaSphere();
        }

        UpdateInertiaTensor();

        UpdateBoundings();
    }

    void Collider::SetMass(const float mass)
    {
        m_mass_ = mass;
    }

    void Collider::SetOffset(const Vector3& offset)
    {
        m_offset_ = offset;
    }

    void Collider::SetMesh(const WeakMesh& mesh)
    {
        if (const auto locked = mesh.lock())
        {
            m_mesh_ = mesh;
            GenerateFromMesh(mesh);
            m_mesh_name_ = locked->GetName();
        }
    }

    bool Collider::Intersects(Collider& other) const
    {
        if (m_type_ == BOUNDING_TYPE_BOX)
        {
            return other.Intersects_GENERAL_TYPE(m_boundings_.box);
        }
        if (m_type_ == BOUNDING_TYPE_SPHERE)
        {
            return other.Intersects_GENERAL_TYPE(m_boundings_.sphere);
        }

        return false;
    }

    bool Collider::Intersects(
        const Ray& ray, float distance,
        float&     intersection) const
    {
        if (m_type_ == BOUNDING_TYPE_BOX)
        {
            const Vector3 Extents = m_boundings_.box.Extents;
            const auto    test    = Physics::Raycast::TestRayOBBIntersection(
                                                                             ray.position, ray.direction, -Extents,
                                                                             Extents,
                                                                             m_world_matrix_,
                                                                             intersection);

            return test && intersection <= distance;
        }
        if (m_type_ == BOUNDING_TYPE_SPHERE)
        {
            const auto test = Physics::Raycast::TestRaySphereIntersection(
                                                                          ray, m_boundings_.sphere.Center,
                                                                          m_boundings_.sphere.Radius,
                                                                          intersection);

            return test && intersection <= distance;
        }

        return false;
    }

    bool Collider::Contains(Collider& other) const
    {
        if (m_type_ == BOUNDING_TYPE_BOX)
        {
            return other.Contains_GENERAL_TYPE(m_boundings_.box);
        }
        if (m_type_ == BOUNDING_TYPE_SPHERE)
        {
            return other.Contains_GENERAL_TYPE(m_boundings_.sphere);
        }

        return false;
    }

    void Collider::AddCollidedObject(const EntityID id)
    {
        m_collided_objects_.insert(id);

        if (!m_collision_count_.contains(id))
        {
            m_collision_count_.insert({id, 1});
        }
        else
        {
            m_collision_count_[id]++;
        }
    }

    void Collider::AddSpeculationObject(const EntityID id)
    {
        m_speculative_collision_candidates_.insert(id);
    }

    void Collider::RemoveCollidedObject(const EntityID id)
    {
        m_collided_objects_.erase(id);
    }

    void Collider::RemoveSpeculationObject(const EntityID id)
    {
        m_speculative_collision_candidates_.erase(id);
    }

    bool Collider::IsCollidedObject(const EntityID id) const
    {
        return m_collided_objects_.contains(id);
    }

    const std::set<EntityID>& Collider::GetCollidedObjects() const
    {
        return m_collided_objects_;
    }

    const std::set<EntityID>& Collider::GetSpeculation() const
    {
        return m_speculative_collision_candidates_;
    }

    bool Collider::GetDirtyFlag() const
    {
        return m_bDirtyByTransform;
    }

    Vector3 Collider::GetPreviousPosition() const
    {
        return m_previous_position_;
    }

    Vector3 Collider::GetPosition() const
    {
        return m_position_;
    }

    Quaternion Collider::GetRotation() const
    {
        return m_rotation_;
    }

    Vector3 Collider::GetSize() const
    {
        return m_size_;
    }

    void Collider::GetPenetration(
        const Collider& other, Vector3& normal,
        float&          depth) const
    {
        auto dir = other.GetPosition() - GetPosition();
        dir.Normalize();

        Physics::GJK::GJKAlgorithm(*this, other, dir, normal, depth);
    }

    UINT Collider::GetCollisionCount(const EntityID id) const
    {
        if (!m_collision_count_.contains(id))
        {
            return 0;
        }

        return m_collision_count_.at(id);
    }

    float Collider::GetMass() const
    {
        return m_mass_;
    }

    float Collider::GetInverseMass() const
    {
        return 1.0f / m_mass_;
    }

    XMFLOAT3X3 Collider::GetInertiaTensor() const
    {
        return m_inertia_tensor_;
    }

    eBoundingType Collider::GetType() const
    {
        return m_type_;
    }

    const Matrix& Collider::GetWorldMatrix() const
    {
        return m_world_matrix_;
    }

    Collider::Collider()
    : Component(COMPONENT_PRIORITY_COLLIDER, {}),
      m_bDirtyByTransform(false),
      m_previous_position_(Vector3::Zero),
      m_position_(Vector3::Zero),
      m_size_(Vector3::One),
      m_rotation_(Quaternion::Identity),
      m_offset_(Vector3::Zero),
      m_type_(BOUNDING_TYPE_BOX),
      m_mass_(1.f),
      m_boundings_({}),
      m_inertia_tensor_(),
      m_world_matrix_()
    {}

    void Collider::FixedUpdate(const float& dt) {}

    void Collider::OnDeserialized()
    {
        Component::OnDeserialized();

        const auto euler = m_rotation_.ToEuler();

        m_yaw_pitch_roll_degree_ =
                Vector3(
                        XMConvertToDegrees(euler.y), XMConvertToDegrees(euler.x),
                        XMConvertToDegrees(euler.z));

        InitializeStockVertices();
        if (!m_mesh_name_.empty())
        {
            m_mesh_ = GetResourceManager().GetResource<Resources::Mesh>(m_mesh_name_);
        }

        if (m_type_ == BOUNDING_TYPE_BOX)
        {
            GenerateInertiaCube();
        }
        else if (m_type_ == BOUNDING_TYPE_SPHERE)
        {
            GenerateInertiaSphere();
        }

        UpdateInertiaTensor();
        UpdateBoundings();
    }

    void Collider::OnImGui()
    {
        Component::OnImGui();
        ImGui::Indent(2);
        ImGui::Checkbox("Dirty by Transform", &m_bDirtyByTransform);

        ImGui::Text("Previous Position");
        ImGuiVector3Editable(GetID(), "previous_position", m_previous_position_);

        ImGui::Text("Position");
        ImGuiVector3Editable(GetID(), "position", m_position_);

        ImGui::Text("Size");
        ImGuiVector3Editable(GetID(), "size", m_size_);

        ImGui::Text("Rotation");
        ImGuiQuaternionEditable(GetID(), "rotation", m_rotation_);

        ImGui::InputInt("Collider Type", reinterpret_cast<int*>(&m_type_));

        ImGui::Text("Collider Mesh : %s", m_mesh_name_.c_str());
        ImGui::InputFloat("Collider Mass", &m_mass_);

        // TODO: colliding objects
    }

    TypeName Collider::GetVirtualTypeName() const
    {
        return typeid(Collider).name();
    }

    void Collider::Render(const float& dt)
    {
#ifdef _DEBUG
        if (m_collided_objects_.empty())
        {
            GetDebugger().Draw(m_type_, Colors::OrangeRed, m_boundings_);
        }
        else
        {
            GetDebugger().Draw(m_type_, Colors::GreenYellow, m_boundings_);
        }
#endif

        m_previous_position_ = m_position_;
    }

    void Collider::PostRender(const float& dt) {}

    void Collider::UpdateFromTransform()
    {
        if (const auto tr = GetOwner().lock()->GetComponent<Transform>().lock(); m_bDirtyByTransform)
        {
            m_position_ = tr->GetPosition();
            m_rotation_ = tr->GetRotation();
            m_size_     = tr->GetScale();

            UpdateBoundings();
        }
    }

    void Collider::UpdateBoundings()
    {
        if (m_type_ == BOUNDING_TYPE_BOX)
        {
            SetPosition_GENERAL_TYPE(m_boundings_.box, m_position_);
            SetSize_GENERAL_TYPE(m_boundings_.box, m_size_);
            SetRotation_GENERAL_TYPE(m_boundings_.box, m_rotation_);
        }
        else if (m_type_ == BOUNDING_TYPE_SPHERE)
        {
            SetPosition_GENERAL_TYPE(m_boundings_.sphere, m_position_);
            SetSize_GENERAL_TYPE(m_boundings_.sphere, m_size_);
            m_rotation_ = Quaternion::Identity;
        }

        m_world_matrix_ = Matrix::CreateWorld(Vector3::Zero, g_forward, Vector3::Up);
        m_world_matrix_ *= Matrix::CreateScale(m_size_);
        m_world_matrix_ *= Matrix::CreateFromQuaternion(m_rotation_);
        m_world_matrix_ *= Matrix::CreateTranslation(m_position_ + m_offset_);
    }

    void Collider::UpdateInertiaTensor()
    {
        const Quaternion rotation = m_rotation_;
        Quaternion       conjugate;

        rotation.Conjugate(conjugate);
        const Matrix invOrientation = Matrix::CreateFromQuaternion(conjugate);
        const Matrix orientation    = Matrix::CreateFromQuaternion(rotation);

        const Matrix matrix = orientation *
                              XMMatrixScaling(GetSize().x, GetSize().y, GetSize().z) *
                              invOrientation;

        XMStoreFloat3x3(&m_inertia_tensor_, matrix);
    }

    Collider::Collider(const WeakObject& owner, const WeakMesh& mesh)
    : Component(COMPONENT_PRIORITY_COLLIDER, owner),
      m_bDirtyByTransform(false),
      m_position_(Vector3::Zero),
      m_size_(Vector3::One),
      m_rotation_(Quaternion::Identity),
      m_offset_(Vector3::Zero),
      m_type_(BOUNDING_TYPE_BOX),
      m_mass_(1.0f),
      m_boundings_({}),
      m_inertia_tensor_(),
      m_mesh_(mesh)
    {
        if (const auto locked = m_mesh_.lock())
        {
            m_mesh_name_ = locked->GetName();
        }
    }

    void Collider::GenerateInertiaCube()
    {
        const Vector3 dimensions_squared = GetSize() * GetSize();

        m_inverse_inertia_.x = (12.0f * GetInverseMass()) /
                               (dimensions_squared.y + dimensions_squared.z);
        m_inverse_inertia_.y = (12.0f * GetInverseMass()) /
                               (dimensions_squared.x + dimensions_squared.z);
        m_inverse_inertia_.z = (12.0f * GetInverseMass()) /
                               (dimensions_squared.x + dimensions_squared.y);
    }

    void Collider::GenerateInertiaSphere()
    {
        const float radius = GetSize().x;
        const float i      = 2.5f * GetInverseMass() / (radius * radius);

        m_inverse_inertia_ = Vector3(i, i, i);
    }

    void Collider::PreUpdate(const float& dt)
    {
        static float second_counter = 0.f;

        if (second_counter >= 1.f)
        {
            m_collision_count_.clear();
        }

        second_counter += dt;

        UpdateFromTransform();
        UpdateInertiaTensor();
    }

    void Collider::Update(const float& dt)
    {
        UpdateFromTransform();
        UpdateInertiaTensor();
    }

    void Collider::PreRender(const float& dt)
    {
        m_rotation_ = Quaternion::CreateFromYawPitchRoll(
                                                         XMConvertToRadians(m_yaw_pitch_roll_degree_.x),
                                                         XMConvertToRadians(m_yaw_pitch_roll_degree_.y),
                                                         XMConvertToRadians(m_yaw_pitch_roll_degree_.z));
    }
} // namespace Engine::Component
