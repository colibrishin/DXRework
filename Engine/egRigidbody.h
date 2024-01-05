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

        void SetGravityOverride(bool gravity);
        void SetGrounded(bool grounded);
        void SetFrictionCoefficient(float mu);
        void SetFixed(bool fixed);

        void SetLinearMomentum(const Vector3& velocity);
        void SetAngularMomentum(const Vector3& velocity);
        void SetLinearFriction(const Vector3& friction);
        void SetAngularFriction(const Vector3& friction);

        void SetDragForce(const Vector3& drag);

        void AddLinearMomentum(const Vector3& velocity);
        void AddAngularMomentum(const Vector3& velocity);
        void AddForce(const Vector3& force);
        void AddTorque(const Vector3& torque);

        float GetFrictionCoefficient() const;

        Vector3 GetLinearMomentum() const;
        Vector3 GetAngularMomentum() const;
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

        Vector3 m_linear_momentum_;
        Vector3 m_angular_momentum_;

        Vector3 m_linear_friction_;
        Vector3 m_angular_friction_;
        Vector3 m_drag_force_;

        Vector3 m_force_;
        Vector3 m_torque_;
    };
} // namespace Engine::Component

BOOST_CLASS_EXPORT_KEY(Engine::Components::Rigidbody)
