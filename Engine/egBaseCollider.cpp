#include "pch.h"
#include "egBaseCollider.hpp"
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
                       Engine::Components::BaseCollider,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Component))
                       _ARTAG(m_type_) _ARTAG(m_model_name_) _ARTAG(m_mass_)
                       _ARTAG(m_boundings_))

namespace Engine::Components
{
    const std::vector<Graphics::VertexElement>& BaseCollider::GetVertices() const
    {
        if (const auto model = m_model_.lock())
        {
            return model->GetVertices();
        }

        if (m_type_ == BOUNDING_TYPE_BOX)
        {
            return m_cube_stock_;
        }
        if (m_type_ == BOUNDING_TYPE_SPHERE)
        {
            return m_sphere_stock_;
        }

        return {};
    }

    Matrix BaseCollider::GetWorldMatrix() const
    {
        return GetLocalMatrix() * GetOwner().lock()->GetComponent<Transform>().lock()->GetWorldMatrix();
    }

    Matrix BaseCollider::GetLocalMatrix() const
    {
        return m_local_matrix_;
    }

    void BaseCollider::Initialize()
    {
        Component::Initialize();

        InitializeStockVertices();

        const auto vtx_ptr = reinterpret_cast<Vector3*>(m_cube_stock_.data());
        m_boundings_.CreateFromPoints<BoundingBox>(m_cube_stock_.size(), vtx_ptr, sizeof(Graphics::VertexElement));

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

    void BaseCollider::InitializeStockVertices()
    {
        GeometricPrimitive::IndexCollection  index;
        GeometricPrimitive::VertexCollection vertex;

        if (m_cube_stock_.empty())
        {
            GeometricPrimitive::CreateCube(vertex, index, 1.f, false);

            for (const auto& v : vertex)
            {
                m_cube_stock_.push_back({v.position});
            }
        }
        if (m_sphere_stock_.empty())
        {
            GeometricPrimitive::CreateSphere(vertex, index, 1.f, 16, false);

            for (const auto& v : vertex)
            {
                m_sphere_stock_.push_back({v.position});
            }
        }
    }

    void BaseCollider::FromMatrix(Matrix& mat)
    {
        m_local_matrix_ = mat;
    }

    void BaseCollider::SetType(const eBoundingType type)
    {
        m_type_ = type;

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

    void BaseCollider::SetMass(const float mass)
    {
        m_mass_ = mass;
    }

    void BaseCollider::SetBoundingBox(const BoundingOrientedBox& bounding)
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

        UpdateInertiaTensor();
    }

    void BaseCollider::SetModel(const WeakModel& model)
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

    bool BaseCollider::Intersects(const StrongBaseCollider& lhs, const StrongBaseCollider& rhs, const Vector3& offset)
    {
        if (lhs->m_type_ == BOUNDING_TYPE_BOX)
        {
            static BoundingOrientedBox box;
            box = lhs->GetBounding<BoundingOrientedBox>();
            box.Center = box.Center + offset;
            return rhs->Intersects_GENERAL_TYPE(box);
        }
        else if (lhs->m_type_ == BOUNDING_TYPE_SPHERE)
        {
            static BoundingSphere sphere;
            sphere = lhs->GetBounding<BoundingSphere>();
            sphere.Center = sphere.Center + offset;
            return rhs->Intersects_GENERAL_TYPE(sphere);
        }

        return false;
    }

    bool BaseCollider::Intersects(const StrongBaseCollider& other) const
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

    bool BaseCollider::Intersects(
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
                                                                       GetWorldMatrix(),
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

    bool BaseCollider::Contains(const StrongBaseCollider& other) const
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

    void BaseCollider::AddCollidedObject(const GlobalEntityID id)
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

    void BaseCollider::AddSpeculationObject(const GlobalEntityID id)
    {
        std::lock_guard lock(m_speculative_mutex_);
        m_speculative_collision_candidates_.insert(id);
    }

    void BaseCollider::RemoveCollidedObject(const GlobalEntityID id)
    {
        std::lock_guard lock(m_collision_mutex_);
        m_collided_objects_.erase(id);
    }

    void BaseCollider::RemoveSpeculationObject(const GlobalEntityID id)
    {
        std::lock_guard lock(m_speculative_mutex_);
        m_speculative_collision_candidates_.erase(id);
    }

    bool BaseCollider::IsCollidedObject(const GlobalEntityID id)
    {
        std::lock_guard lock(m_collision_mutex_);
        return m_collided_objects_.contains(id);
    }

    std::set<GlobalEntityID> BaseCollider::GetCollidedObjects()
    {
        std::lock_guard lock(m_collision_mutex_);
        return m_collided_objects_;
    }

    std::set<GlobalEntityID> BaseCollider::GetSpeculation()
    {
        std::lock_guard lock(m_speculative_mutex_);
        return m_speculative_collision_candidates_;
    }

    void BaseCollider::GetPenetration(
        const BaseCollider& other, Vector3& normal,
        float&          depth) const
    {
        auto dir = other.GetWorldMatrix().Translation() - GetWorldMatrix().Translation();
        dir.Normalize();

        Physics::GJK::GJKAlgorithm(
                                   GetWorldMatrix(), other.GetWorldMatrix(), GetVertices(), other.GetVertices(), dir,
                                   normal, depth);
    }

    UINT BaseCollider::GetCollisionCount(const GlobalEntityID id)
    {
        std::lock_guard lock(m_collision_count_mutex_);
        if (!m_collision_count_.contains(id))
        {
            return 0;
        }

        return m_collision_count_.at(id);
    }

    float BaseCollider::GetMass() const
    {
        return m_mass_;
    }

    float BaseCollider::GetInverseMass() const
    {
        return 1.0f / m_mass_;
    }

    XMFLOAT3X3 BaseCollider::GetInertiaTensor() const
    {
        return m_inertia_tensor_;
    }

    eBoundingType BaseCollider::GetType() const
    {
        return m_type_;
    }

    BaseCollider::BaseCollider()
    : Component(COM_T_COLLIDER, {}),
      m_type_(BOUNDING_TYPE_BOX),
      m_mass_(1.f),
      m_boundings_(),
      m_inertia_tensor_() {}

    void BaseCollider::FixedUpdate(const float& dt) {}

    void BaseCollider::OnDeserialized()
    {
        Component::OnDeserialized();

        InitializeStockVertices();

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

    void BaseCollider::OnImGui()
    {
        Component::OnImGui();
        ImGui::Indent(2);

        ImGui::InputInt("Collider Type", reinterpret_cast<int*>(&m_type_));

        ImGui::InputFloat("Collider Mass", &m_mass_);

        // TODO: colliding objects
    }

    void BaseCollider::UpdateInertiaTensor()
    {
        Quaternion rotation;

        if (m_type_ == BOUNDING_TYPE_BOX)
        {
            rotation = GetBounding<BoundingOrientedBox>().Orientation;
        }
        else if (m_type_ == BOUNDING_TYPE_SPHERE)
        {
            rotation = Quaternion::Identity;
        }

        Quaternion       conjugate;

        rotation.Conjugate(conjugate);
        const Matrix invOrientation = Matrix::CreateFromQuaternion(conjugate);
        const Matrix orientation    = Matrix::CreateFromQuaternion(rotation);

        Vector3 dimensions;

        if (m_type_ == BOUNDING_TYPE_BOX)
        {
            dimensions = Vector3(GetBounding<BoundingOrientedBox>().Extents) * 2;
        }
        else if (m_type_ == BOUNDING_TYPE_SPHERE)
        {
            dimensions = Vector3(GetBounding<BoundingSphere>().Radius) * 2;
        }

        const Matrix matrix = orientation *
            Matrix::CreateScale(dimensions) * invOrientation;

        XMStoreFloat3x3(&m_inertia_tensor_, matrix);
    }

    BaseCollider::BaseCollider(const WeakObject& owner)
    : Component(COM_T_COLLIDER, owner),
      m_type_(BOUNDING_TYPE_BOX),
      m_mass_(1.0f),
      m_boundings_(),
      m_inertia_tensor_()
    {
    }

    void BaseCollider::GenerateInertiaCube()
    {
        const Vector3 dim                = Vector3(GetBounding<BoundingOrientedBox>().Extents) * 2;
        const Vector3 dimensions_squared = dim * dim;

        m_inverse_inertia_.x = (12.0f * GetInverseMass()) /
                               (dimensions_squared.y + dimensions_squared.z);
        m_inverse_inertia_.y = (12.0f * GetInverseMass()) /
                               (dimensions_squared.x + dimensions_squared.z);
        m_inverse_inertia_.z = (12.0f * GetInverseMass()) /
                               (dimensions_squared.x + dimensions_squared.y);
    }

    void BaseCollider::GenerateInertiaSphere()
    {
        const float radius = GetBounding<BoundingSphere>().Radius;
        const float i      = 2.5f * GetInverseMass() / (radius * radius);

        m_inverse_inertia_ = Vector3(i, i, i);
    }

    void BaseCollider::PreUpdate(const float& dt)
    {
        static float second_counter = 0.f;

        if (second_counter >= 1.f)
        {
            std::lock_guard lock(m_collision_count_mutex_);
            m_collision_count_.clear();
        }

        second_counter += dt;

        UpdateInertiaTensor();
    }

    void BaseCollider::Update(const float& dt)
    {
        UpdateInertiaTensor();
    }

    void BaseCollider::PostUpdate(const float& dt)
    {
        Component::PostUpdate(dt);
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
