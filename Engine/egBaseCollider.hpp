#pragma once
#include "egCommon.hpp"
#include "egComponent.h"
#include "egGenericBounding.hpp"
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

    void FromMatrix(const Matrix& mat);

    void SetType(eBoundingType type);
    void SetMass(float mass);

    void SetBoundingBox(const BoundingOrientedBox& bounding);
    void SetModel(const WeakShape& model);

    static bool Intersects(const StrongCollider& lhs, const StrongCollider& rhs, const Vector3& dir);
    static bool Intersects(const StrongCollider& lhs, const StrongCollider& rhs, float epsilon = g_epsilon);
    static bool Intersects(const StrongCollider& lhs, const StrongCollider& rhs, float dist, const Vector3& dir);

    bool Intersects(const Vector3& start, const Vector3& dir, float distance, float& intersection) const;

    void AddCollidedObject(GlobalEntityID id);
    void RemoveCollidedObject(GlobalEntityID id);

    bool                            IsCollidedObject(GlobalEntityID id) const;
    const std::set<GlobalEntityID>& GetCollidedObjects() const;
    UINT                            GetCPS(GlobalEntityID id) const;
    UINT                            GetCollisionCount(GlobalEntityID id) const;

    bool GetPenetration(
      const Collider& other, Vector3& normal,
      float&          depth
    ) const;

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

    void OnDeserialized() override;
    void OnImGui() override;

    Physics::GenericBounding GetBounding() const;

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

    void UpdateInertiaTensor();
    void GenerateInertiaCube();
    void GenerateInertiaSphere();

    eBoundingType m_type_;
    EntityName    m_model_name_;

    Physics::GenericBounding m_boundings_;

    float m_mass_;

    // Non-serialized
    inline static std::vector<Graphics::VertexElement> m_cube_stock_   = {};
    inline static std::vector<Graphics::VertexElement> m_sphere_stock_ = {};

    std::set<GlobalEntityID> m_collided_objects_;
    // Collision per second
    std::map<GlobalEntityID, UINT> m_cps_;
    // Long-term collision count
    std::map<GlobalEntityID, UINT> m_ltcc_;

    Vector3    m_inverse_inertia_;
    XMFLOAT3X3 m_inertia_tensor_;
    Matrix     m_local_matrix_;

    WeakShape m_model_;
  };
} // namespace Engine::Component

BOOST_CLASS_EXPORT_KEY(Engine::Components::Collider)
