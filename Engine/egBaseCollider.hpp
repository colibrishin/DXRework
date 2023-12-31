#pragma once
#include "egBoundingGroup.hpp"
#include "egCommon.hpp"
#include "egComponent.h"
#include "egHelper.hpp"
#include "egTransform.h"

namespace Engine::Components
{
    using namespace DirectX;

    class Collider final : public Abstract::Component
    {
    public:
        COMPONENT_T(COM_T_COLLIDER)

        Collider(const WeakObject& owner);
        ~Collider() override = default;

        void FromMatrix(Matrix& mat);

        void SetType(eBoundingType type);
        void SetMass(const float mass);

        void SetBoundingBox(const BoundingOrientedBox & bounding);
        void SetModel(const WeakModel& model);

        static bool Intersects(const StrongCollider& lhs, const StrongCollider& rhs, const Vector3& dir);
        static bool Intersects(const StrongCollider& lhs, const StrongCollider& rhs, const float epsilon = g_epsilon);
        bool        Intersects(const StrongCollider& other) const;
        bool        Intersects(const Ray& ray, float distance, float& intersection) const;
        bool        Contains(const StrongCollider & other) const;

        void AddCollidedObject(GlobalEntityID id);
        void AddSpeculationObject(GlobalEntityID id);

        void RemoveCollidedObject(const GlobalEntityID id);
        void RemoveSpeculationObject(const GlobalEntityID id);

        bool                     IsCollidedObject(const GlobalEntityID id);
        std::set<GlobalEntityID> GetCollidedObjects();
        std::set<GlobalEntityID> GetSpeculation();
        UINT                     GetCollisionCount(GlobalEntityID id);

        void GetPenetration(
            const Collider& other, Vector3& normal,
            float&          depth) const;

        float      GetMass() const;
        float      GetInverseMass() const;
        XMFLOAT3X3 GetInertiaTensor() const;

        eBoundingType GetType() const;

        const std::vector<Graphics::VertexElement>& GetVertices() const;
        Matrix                                      GetWorldMatrix() const;
        virtual Matrix                              GetLocalMatrix() const;

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
                return m_boundings_.As<BoundingOrientedBox>(GetWorldMatrix());
            }
            else if constexpr (std::is_same_v<T, BoundingSphere>)
            {
                return m_boundings_.As<BoundingSphere>(GetWorldMatrix());
            }
            else
            {
                static_assert("Invalid type");
                throw std::exception("Invalid type");
            }
        }

        template <typename T>
        T GetBoundingLocal() const
        {
            if constexpr (std::is_same_v<T, BoundingOrientedBox>)
            {
                return m_boundings_.As<BoundingOrientedBox>(GetLocalMatrix());
            }
            else if constexpr (std::is_same_v<T, BoundingSphere>)
            {
                return m_boundings_.As<BoundingSphere>(GetLocalMatrix());
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

        template <typename T>
        bool Intersects_GENERAL_TYPE(const T& other)
        {
            if (m_type_ == BOUNDING_TYPE_BOX)
            {
                BoundingOrientedBox box = GetBounding<BoundingOrientedBox>();
                box.Extents = box.Extents + (Vector3::One * g_epsilon);
                return box.Intersects(other);
            }
            if (m_type_ == BOUNDING_TYPE_SPHERE)
            {
                BoundingSphere sphere = GetBounding<BoundingSphere>();
                sphere.Radius += g_epsilon;
                return sphere.Intersects(other);
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

        eBoundingType m_type_;
        EntityName    m_model_name_;

        Physics::BoundingGroup m_boundings_;

        float m_mass_;

        // Non-serialized
        inline static std::vector<Graphics::VertexElement> m_cube_stock_   = {};
        inline static std::vector<Graphics::VertexElement> m_sphere_stock_ = {};

        std::mutex                   m_collision_mutex_;
        std::mutex                   m_collision_count_mutex_;
        std::mutex                   m_speculative_mutex_;

        std::set<GlobalEntityID>     m_collided_objects_;
        std::map<GlobalEntityID, UINT> m_collision_count_;
        std::set<GlobalEntityID>     m_speculative_collision_candidates_;

        Vector3    m_inverse_inertia_;
        XMFLOAT3X3 m_inertia_tensor_;
        Matrix     m_local_matrix_;

        WeakModel m_model_;
    };
} // namespace Engine::Component

BOOST_CLASS_EXPORT_KEY(Engine::Components::Collider)
