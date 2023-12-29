#include "pch.h"
#include "egCollider.hpp"
#include <imgui_stdlib.h>
#include "egCollision.h"
#include "egCubeMesh.h"
#include "egD3Device.hpp"
#include "egResourceManager.hpp"
#include "egSceneManager.hpp"
#include "egSphereMesh.h"
#include "egTransform.h"
#include "egShape.h"

#include "egManagerHelper.hpp"

SERIALIZER_ACCESS_IMPL(
                       Engine::Components::Collider,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Component))
                       _ARTAG(m_bDirtyByTransform) _ARTAG(m_position_)
                       _ARTAG(m_rotation_) _ARTAG(m_size_)
                       _ARTAG(m_type_) _ARTAG(m_mass_)
                       _ARTAG(m_mesh_name_))

namespace Engine::Components
{
    void Collider::SetOffsetPosition(const Vector3& position)
    {
        Vector3CheckNanException(position);
        m_position_ = position;
        UpdateWorldMatrix();
    }

    void Collider::SetOffsetRotation(const Quaternion& rotation)
    {
        m_rotation_ = rotation;
        UpdateInertiaTensor();
        UpdateWorldMatrix();
    }

    void Collider::SetYawPitchRoll(const Vector3& yaw_pitch_roll)
    {
        m_yaw_pitch_roll_degree_ = yaw_pitch_roll;
        m_rotation_              =
                Quaternion::CreateFromYawPitchRoll(
                                                   XMConvertToRadians(yaw_pitch_roll.x),
                                                   XMConvertToRadians(yaw_pitch_roll.y),
                                                   XMConvertToRadians(yaw_pitch_roll.z));
        UpdateInertiaTensor();
    }

    const std::vector<const Vector3*>& Collider::GetVertices() const
    {
        if (const auto model = m_model_.lock())
        {
            return model->GetVertices();
        }

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

        m_boundings_.CreateFromPoints<BoundingBox>(m_cube_stock_.size(), m_cube_stock_.data(), sizeof(Vector3));

        if (m_type_ == BOUNDING_TYPE_BOX)
        {
            GenerateInertiaCube();
        }
        else if (m_type_ == BOUNDING_TYPE_SPHERE)
        {
            GenerateInertiaSphere();
        }

        UpdateInertiaTensor();
        UpdateWorldMatrix();
    }

    void Collider::InitializeStockVertices()
    {
        GeometricPrimitive::IndexCollection  index;
        GeometricPrimitive::VertexCollection vertex;

        if (m_cube_stock_.empty())
        {
            GeometricPrimitive::CreateCube(vertex, index, 1.f, false);

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
                              [&](const Graphics::VertexElement& shape)
                              {
                                  serialized_vertices.emplace_back(shape.position);
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

    void Collider::SetOffsetSize(const Vector3& size)
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

        UpdateWorldMatrix();
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
    }

    void Collider::SetMass(const float mass)
    {
        m_mass_ = mass;
    }

    void Collider::SetBoundingBox(const BoundingBox& bounding)
    {
        m_boundings_.UpdateFromBoundingBox(bounding);

        if (m_type_ == BOUNDING_TYPE_BOX)
        {
            GenerateInertiaCube();
        }
        else if (m_type_ == BOUNDING_TYPE_SPHERE)
        {
            GenerateInertiaSphere();
        }
    }

    void Collider::SetMesh(const WeakMesh& mesh)
    {
        if (const auto locked = mesh.lock())
        {
            m_mesh_name_ = locked->GetName();
            m_mesh_      = mesh;
            SetBoundingBox(locked->GetBoundingBox());
        }
    }

    void Collider::SetModel(const WeakModel& model)
    {
        if (const auto locked = model.lock())
        {
            m_model_name_ = locked->GetName();
            m_model_      = locked;
            SetBoundingBox(locked->GetBoundingBox());
        }
    }

    bool Collider::Intersects(const StrongCollider& lhs, const StrongCollider& rhs, const Vector3& offset)
    {
        if (lhs->m_type_ == BOUNDING_TYPE_BOX)
        {
            static BoundingOrientedBox box = lhs->GetBounding<BoundingOrientedBox>();
            box.Center = box.Center + offset;
            return rhs->Intersects_GENERAL_TYPE(box);
        }
        else if (lhs->m_type_ == BOUNDING_TYPE_SPHERE)
        {
            static BoundingSphere sphere = lhs->GetBounding<BoundingSphere>();
            sphere.Center = sphere.Center + offset;
            return rhs->Intersects_GENERAL_TYPE(sphere);
        }

        return false;
    }

    bool Collider::Intersects(const StrongCollider& other) const
    {
        if (m_type_ == BOUNDING_TYPE_BOX)
        {
            return other->Intersects_GENERAL_TYPE(GetBounding<BoundingOrientedBox>());
        }
        if (m_type_ == BOUNDING_TYPE_SPHERE)
        {
            return other->Intersects_GENERAL_TYPE(GetBounding<BoundingSphere>());
        }

        return false;
    }

    bool Collider::Intersects(
        const Ray& ray, float distance,
        float&     intersection) const
    {
        if (m_type_ == BOUNDING_TYPE_BOX)
        {
            const auto    box     = GetBounding<BoundingOrientedBox>();
            const Vector3 Extents = box.Extents;
            const auto    test    = Physics::Raycast::TestRayOBBIntersection(
                                                                       ray.position, ray.direction, -Extents,
                                                                       Extents,
                                                                       m_world_matrix_,
                                                                       intersection);

            return test && intersection <= distance;
        }
        if (m_type_ == BOUNDING_TYPE_SPHERE)
        {
            const auto sphere = GetBounding<BoundingSphere>();
            const auto test = Physics::Raycast::TestRaySphereIntersection(
                                                                          ray, sphere.Center,
                                                                          sphere.Radius,
                                                                          intersection);

            return test && intersection <= distance;
        }

        return false;
    }

    bool Collider::Contains(const StrongCollider& other) const
    {
        if (m_type_ == BOUNDING_TYPE_BOX)
        {
            return other->Contains_GENERAL_TYPE(GetBounding<BoundingOrientedBox>());
        }
        if (m_type_ == BOUNDING_TYPE_SPHERE)
        {
            return other->Contains_GENERAL_TYPE(GetBounding<BoundingSphere>());
        }

        return false;
    }

    void Collider::AddCollidedObject(const GlobalEntityID id)
    {
        std::lock_guard lock(m_collision_mutex_);
        m_collided_objects_.insert(id);

        std::lock_guard lock2(m_collision_count_mutex_);
        if (!m_collision_count_.contains(id))
        {
            m_collision_count_.insert({id, 1});
        }
        else
        {
            m_collision_count_[id]++;
        }
    }

    void Collider::AddSpeculationObject(const GlobalEntityID id)
    {
        std::lock_guard lock(m_speculative_mutex_);
        m_speculative_collision_candidates_.insert(id);
    }

    void Collider::RemoveCollidedObject(const GlobalEntityID id)
    {
        std::lock_guard lock(m_collision_mutex_);
        m_collided_objects_.erase(id);
    }

    void Collider::RemoveSpeculationObject(const GlobalEntityID id)
    {
        std::lock_guard lock(m_speculative_mutex_);
        m_speculative_collision_candidates_.erase(id);
    }

    bool Collider::IsCollidedObject(const GlobalEntityID id)
    {
        std::lock_guard lock(m_collision_mutex_);
        return m_collided_objects_.contains(id);
    }

    std::set<GlobalEntityID> Collider::GetCollidedObjects()
    {
        std::lock_guard lock(m_collision_mutex_);
        return m_collided_objects_;
    }

    std::set<GlobalEntityID> Collider::GetSpeculation()
    {
        std::lock_guard lock(m_speculative_mutex_);
        return m_speculative_collision_candidates_;
    }

    Vector3 Collider::GetSize() const
    {
        return m_size_;
    }

    void Collider::GetPenetration(
        const Collider& other, Vector3& normal,
        float&          depth) const
    {
        auto dir = other.GetTotalPosition() - GetTotalPosition();
        dir.Normalize();

        Physics::GJK::GJKAlgorithm(
                                   GetWorldMatrix(), other.GetWorldMatrix(), GetVertices(), other.GetVertices(), dir,
                                   normal, depth);
    }

    UINT Collider::GetCollisionCount(const GlobalEntityID id)
    {
        std::lock_guard lock(m_collision_count_mutex_);
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
    : Component(COM_T_COLLIDER, {}),
      m_bDirtyByTransform(false),
      m_position_(Vector3::Zero),
      m_size_(Vector3::One),
      m_rotation_(Quaternion::Identity),
      m_type_(BOUNDING_TYPE_BOX),
      m_mass_(1.f),
      m_boundings_(),
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
            GenerateFromMesh(m_mesh_);
        }

        if (m_type_ == BOUNDING_TYPE_BOX)
        {
            GenerateInertiaCube();
        }
        else if (m_type_ == BOUNDING_TYPE_SPHERE)
        {
            GenerateInertiaSphere();
        }

        UpdateWorldMatrix();
        UpdateInertiaTensor();
    }

    void Collider::OnImGui()
    {
        Component::OnImGui();
        ImGui::Indent(2);
        ImGui::Checkbox("Dirty by Transform", &m_bDirtyByTransform);

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

    void Collider::UpdateInertiaTensor()
    {
        const Quaternion rotation = m_rotation_;
        Quaternion       conjugate;

        rotation.Conjugate(conjugate);
        const Matrix invOrientation = Matrix::CreateFromQuaternion(conjugate);
        const Matrix orientation    = Matrix::CreateFromQuaternion(rotation);

        const Matrix matrix = orientation *
                              Matrix::CreateScale(GetTotalSize()) *
                              invOrientation;

        XMStoreFloat3x3(&m_inertia_tensor_, matrix);
    }

    Collider::Collider(const WeakObject& owner, const WeakMesh& mesh)
    : Component(COM_T_COLLIDER, owner),
      m_bDirtyByTransform(false),
      m_position_(Vector3::Zero),
      m_size_(Vector3::One),
      m_rotation_(Quaternion::Identity),
      m_type_(BOUNDING_TYPE_BOX),
      m_mass_(1.0f),
      m_boundings_(),
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

    void Collider::UpdateWorldMatrix()
    {
        m_world_matrix_ = Matrix::CreateScale(GetTotalSize()) *
                          Matrix::CreateFromQuaternion(GetTotalRotation()) *
                          Matrix::CreateTranslation(GetTotalPosition());
    }

    Vector3 Collider::GetTotalPosition() const
    {
        return GetOwner().lock()->GetComponent<Transform>().lock()->GetWorldPosition() +
               m_position_;
    }

    Quaternion Collider::GetTotalRotation() const
    {
        return GetOwner().lock()->GetComponent<Transform>().lock()->GetWorldRotation() *
               m_rotation_;
    }

    Vector3 Collider::GetTotalSize() const
    {
        return GetOwner().lock()->GetComponent<Transform>().lock()->GetScale() *
               m_size_;
    }

    void Collider::PreUpdate(const float& dt)
    {
        static float second_counter = 0.f;

        if (second_counter >= 1.f)
        {
            std::lock_guard lock(m_collision_count_mutex_);
            m_collision_count_.clear();
        }

        second_counter += dt;

        UpdateWorldMatrix();
        UpdateInertiaTensor();
    }

    void Collider::Update(const float& dt)
    {
        UpdateWorldMatrix();
        UpdateInertiaTensor();
    }

    void Collider::PostUpdate(const float& dt)
    {
        Component::PostUpdate(dt);
        m_rotation_ = Quaternion::CreateFromYawPitchRoll(
                                                         XMConvertToRadians(m_yaw_pitch_roll_degree_.x),
                                                         XMConvertToRadians(m_yaw_pitch_roll_degree_.y),
                                                         XMConvertToRadians(m_yaw_pitch_roll_degree_.z));
#ifdef _DEBUG
        std::lock_guard lock(m_collision_mutex_);
        if (m_collided_objects_.empty())
        {
            if (m_type_ == BOUNDING_TYPE_BOX)
            {
                GetDebugger().Draw(GetBounding<BoundingOrientedBox>(), Colors::OrangeRed);
            }
            else
            {
                GetDebugger().Draw(GetBounding<BoundingSphere>(), Colors::OrangeRed);
            }
        }
        else
        {
            if (m_type_ == BOUNDING_TYPE_BOX)
            {
                GetDebugger().Draw(GetBounding<BoundingOrientedBox>(), Colors::GreenYellow);
            }
            else
            {
                GetDebugger().Draw(GetBounding<BoundingSphere>(), Colors::GreenYellow);
            }
        }
#endif
    }
} // namespace Engine::Component
