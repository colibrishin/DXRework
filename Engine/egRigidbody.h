#pragma once
#include "egCommon.hpp"
#include "egComponent.h"

namespace Engine::Components
{
  class Rigidbody final : public Abstract::Component
  {
  public:
    COMPONENT_T(COM_T_RIDIGBODY)

    explicit Rigidbody(const WeakObjectBase& object);
    Rigidbody(const Rigidbody& other);

    ~Rigidbody() override = default;

    Transform* GetT1() const;

    void SetGravityOverride(bool gravity);
    void SetGrounded(bool grounded);
    void SetFrictionCoefficient(float mu);
    void SetFixed(bool fixed);
    void SetNoAngular(bool no_angular);
    // This will update the T1 from current Transform.
    void Synchronize();

    void SetT0LinearVelocity(const Vector3& v);
    void SetT0AngularVelocity(const Vector3& v);
    void AddLinearImpulse(const Vector3& f);
    void AddAngularImpulse(const Vector3& f);
    void SetLinearFriction(const Vector3& friction);
    void SetAngularFriction(const Vector3& friction);
    void SetDragForce(const Vector3& drag);

    void AddT1Force(const Vector3& force);
    void AddT1Torque(const Vector3& torque);

    float GetFrictionCoefficient() const;

    Vector3 GetT0LinearVelocity() const;
    Vector3 GetT0AngularVelocity() const;
    
    Vector3 GetT0Force() const;
    Vector3 GetT0Torque() const;
    Vector3 GetT1Force() const;
    Vector3 GetT1Torque() const;

    bool GetGrounded() const;
    // Reset state of the rigidbody. T1 force and torque will be now be T0.
    void Reset();
    // Reset state of the rigidbody. Every force and torque will set to be zero.
    void FullReset();

    bool IsGravityAllowed() const;
    bool IsFixed() const;
    bool IsGrounded() const;
    bool GetNoAngular() const;
    bool GetLerp() const;

    void Initialize() override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void FixedUpdate(const float& dt) override;

    void OnSerialized() override;
    void OnDeserialized() override;
    void OnImGui() override;

  protected:
    Rigidbody();

  private:
    SERIALIZE_DECL
    COMP_CLONE_DECL

    bool m_bGrounded;
    bool m_b_no_angular_;
    bool m_bGravityOverride;
    bool m_bFixed;
    bool m_b_lerp_;

    float m_friction_mu_;

    Vector3 m_linear_velocity;
    Vector3 m_angular_velocity;

    Vector3 m_linear_friction_;
    Vector3 m_angular_friction_;
    Vector3 m_drag_force_;

    Vector3 m_t0_force_;
    Vector3 m_t0_torque_;
    Vector3 m_t1_force_;
    Vector3 m_t1_torque_;

    std::unique_ptr<Transform> m_t1_;
  };
} // namespace Engine::Component

BOOST_CLASS_EXPORT_KEY(Engine::Components::Rigidbody)
