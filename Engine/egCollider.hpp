#pragma once
#include "egCommon.hpp"
#include "egComponent.h"
#include "egHelper.hpp"

namespace Engine::Components
{
    using namespace DirectX;

    class Collider final : public Abstract::Component
    {
    public:
        INTERNAL_COMP_CHECK_CONSTEXPR(COM_T_COLLIDER)

        Collider(const WeakObject& owner, const WeakMesh& mesh = {});
        ~Collider() override = default;

        void SetOffsetPosition(const Vector3& position);
        void SetOffsetRotation(const Quaternion& rotation);
        void SetYawPitchRoll(const Vector3& yaw_pitch_roll);
        void SetOffsetSize(const Vector3& size);

        void SetType(eBoundingType type);
        void SetMass(const float mass);

        void SetBoundingBox(const BoundingBox& bounding);
        void SetMesh(const WeakMesh& mesh);
        void SetModel(const WeakModel& model);

        bool Intersects(Collider& other) const;
        bool Intersects(const Ray& ray, float distance, float& intersection) const;
        bool Contains(Collider& other) const;

        void AddCollidedObject(EntityID id);
        void AddSpeculationObject(EntityID id);

        void RemoveCollidedObject(const EntityID id);
        void RemoveSpeculationObject(const EntityID id);

        bool IsCollidedObject(const EntityID id) const;

        const std::set<EntityID>& GetCollidedObjects() const;
        const std::set<EntityID>& GetSpeculation() const;

        Vector3 GetSize() const;

        void GetPenetration(
            const Collider& other, Vector3& normal,
            float&          depth) const;
        UINT GetCollisionCount(EntityID id) const;

        float      GetMass() const;
        float      GetInverseMass() const;
        XMFLOAT3X3 GetInertiaTensor() const;

        eBoundingType GetType() const;

        virtual const std::vector<const Vector3*>& GetVertices() const;
        const Matrix&                              GetWorldMatrix() const;

        void Initialize() override;
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PostUpdate(const float& dt) override;
        void FixedUpdate(const float& dt) override;

        void     OnDeserialized() override;
        void     OnImGui() override;

        template <typename T>
        T GetBounding() const
        {
            if constexpr (std::is_same_v<T, BoundingOrientedBox>)
            {
                return m_boundings_.As<BoundingOrientedBox>(GetTotalSize(), GetTotalRotation(), GetTotalPosition());
            }
            else if constexpr (std::is_same_v<T, BoundingSphere>)
            {
                return m_boundings_.As<BoundingSphere>(GetTotalSize(), GetTotalRotation(), GetTotalPosition());
            }
            else
            {
                static_assert("Invalid type");
                throw std::exception("Invalid type");
            }
        }

    protected:
        Collider();

    private:
        SERIALIZER_ACCESS
        friend class Manager::Physics::LerpManager;

        static void InitializeStockVertices();
        void        GenerateFromMesh(const WeakMesh& mesh);

        template <typename T>
        bool Intersects_GENERAL_TYPE(const T& other)
        {
            if (m_type_ == BOUNDING_TYPE_BOX)
            {
                return GetBounding<BoundingOrientedBox>().Intersects(other);
            }
            if (m_type_ == BOUNDING_TYPE_SPHERE)
            {
                return GetBounding<BoundingSphere>().Intersects(other);
            }

            return false;
        }

        template <typename T>
        bool Contains_GENERAL_TYPE(const T& other)
        {
            if (m_type_ == BOUNDING_TYPE_BOX)
            {
                return GetBounding<BoundingOrientedBox>().Contains(other);
            }
            if (m_type_ == BOUNDING_TYPE_SPHERE)
            {
                return GetBounding<BoundingSphere>().Contains(other);
            }

            return false;
        }

        void UpdateInertiaTensor();
        void GenerateInertiaCube();
        void GenerateInertiaSphere();
        void UpdateWorldMatrix();

        Vector3 GetTotalPosition() const;
        Quaternion GetTotalRotation() const;
        Vector3 GetTotalSize() const;

        bool m_bDirtyByTransform;

        Vector3    m_position_;
        Vector3    m_size_;
        Quaternion m_rotation_;
        Vector3    m_yaw_pitch_roll_degree_;

        eBoundingType m_type_;
        EntityName    m_mesh_name_;
        EntityName    m_model_name_;

        float m_mass_;

        // Non-serialized
        inline static std::vector<Vector3> m_cube_stock_   = {};
        inline static std::vector<Vector3> m_sphere_stock_ = {};

        inline static std::vector<const Vector3*> m_cube_stock_ref_   = {};
        inline static std::vector<const Vector3*> m_sphere_stock_ref_ = {};

        BoundingGroup m_boundings_;

        std::set<EntityID>       m_collided_objects_;
        std::map<EntityID, UINT> m_collision_count_;
        std::set<EntityID>       m_speculative_collision_candidates_;

        Vector3    m_inverse_inertia_;
        XMFLOAT3X3 m_inertia_tensor_;
        Matrix     m_world_matrix_;

        WeakMesh  m_mesh_;
        WeakModel m_model_;
    };
} // namespace Engine::Component

BOOST_CLASS_EXPORT_KEY(Engine::Components::Collider)
