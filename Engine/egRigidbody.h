#pragma once
#include "egCommon.hpp"
#include "egComponent.h"

namespace Engine::Components
{
  class Rigidbody final : public Abstract::Component
  {
  public:
    COMPONENT_T(COM_T_RIDIGBODY)

    explicit Rigidbody(const WeakObject& object);

    ~Rigidbody() override = default;

    Transform* GetT1() const;

    void SetGravityOverride(bool gravity);
    void SetGrounded(bool grounded);
    void SetFrictionCoefficient(float mu);
    void SetFixed(bool fixed);
    // This will update the T1 from current Transform.
    void Synchronize();

    void SetT0LinearVelocity(const Vector3& v);
    void SetT0AngularVelocity(const Vector3& v);
    void AddLinearImpulse(const Vector3& f);
    void AddAngularImpulse(const Vector3& f);
    void SetLinearFriction(const Vector3& friction);
    void SetAngularFriction(const Vector3& friction);
    void SetDragForce(const Vector3& drag);

    void AddForce(const Vector3& force);
    void AddTorque(const Vector3& torque);

    float GetFrictionCoefficient() const;

    Vector3 GetT0LinearVelocity() const;
    Vector3 GetT0AngularVelocity() const;
    Vector3 GetT1LinearVelocity(const float dt) const;
    Vector3 GetT1AngularVelocity(const float dt) const;
    Vector3 GetForce() const;
    Vector3 GetTorque() const;

    bool GetGrounded() const;
    void Reset();
    bool IsGravityAllowed() const;
    bool IsFixed() const;
    bool IsGrounded() const;

    void Initialize() override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void FixedUpdate(const float& dt) override;

    void OnDeserialized() override;
    void OnImGui() override;

  protected:
    Rigidbody();

  private:
    SERIALIZER_ACCESS

    bool m_bGrounded;

    bool m_bGravityOverride;
    bool m_bFixed;

    float m_friction_mu_;

    Vector3 m_linear_velocity;
    Vector3 m_angular_velocity;

    Vector3 m_linear_friction_;
    Vector3 m_angular_friction_;
    Vector3 m_drag_force_;

    Vector3 m_force_;
    Vector3 m_torque_;

    std::unique_ptr<Transform> m_t1_;
  };
} // namespace Engine::Component

BOOST_CLASS_EXPORT_KEY(Engine::Components::Rigidbody)
