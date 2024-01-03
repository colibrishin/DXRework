#include "pch.h"
#include "egRigidbody.h"

#include "egBaseCollider.hpp"
#include "egObject.hpp"
#include "egTransform.h"

SERIALIZER_ACCESS_IMPL(
                       Engine::Components::Rigidbody,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Component)) _ARTAG(m_bGrounded)
                       _ARTAG(m_bGravityOverride) _ARTAG(m_bFixed) _ARTAG(m_friction_mu_)
                       _ARTAG(m_linear_momentum_) _ARTAG(m_angular_momentum_)
                       _ARTAG(m_linear_friction_) _ARTAG(m_angular_friction_)
                       _ARTAG(m_drag_force_) _ARTAG(m_force_) _ARTAG(m_torque_))

namespace Engine::Components
{
    void Rigidbody::Initialize()
    {
        Component::Initialize();

        if (!GetOwner().lock()->GetComponent<Transform>().lock())
        {
            throw std::exception("Rigidbody must have a transform component");
        }

        if (!GetOwner().lock()->GetComponent<BaseCollider>().lock())
        {
            throw std::exception("Rigidbody must have a collider component");
        }
    }

    Rigidbody::Rigidbody(const WeakObject& object)
    : Component(COM_T_RIDIGBODY, object),
      m_bGrounded(false),
      m_bGravityOverride(false),
      m_bFixed(false),
      m_friction_mu_(0.0f) {}

    void Rigidbody::SetGravityOverride(bool gravity)
    {
        m_bGravityOverride = gravity;
    }

    void Rigidbody::SetGrounded(bool grounded)
    {
        m_bGrounded = grounded;
    }

    void Rigidbody::SetFrictionCoefficient(float mu)
    {
        m_friction_mu_ = mu;
    }

    void Rigidbody::SetFixed(bool fixed)
    {
        m_bFixed = fixed;
    }

    void Rigidbody::SetLinearMomentum(const Vector3& velocity)
    {
        Vector3CheckNanException(velocity);
        m_linear_momentum_ = velocity;
    }

    void Rigidbody::SetAngularMomentum(const Vector3& velocity)
    {
        Vector3CheckNanException(velocity);
        m_angular_momentum_ = velocity;
    }

    void Rigidbody::SetLinearFriction(const Vector3& friction)
    {
        m_linear_friction_ = friction;
    }

    void Rigidbody::SetAngularFriction(const Vector3& friction)
    {
        m_angular_friction_ = friction;
    }

    void Rigidbody::SetDragForce(const Vector3& drag)
    {
        m_drag_force_ = drag;
    }

    void Rigidbody::AddLinearMomentum(const Vector3& velocity)
    {
        Vector3CheckNanException(velocity);
        m_linear_momentum_ += velocity;
    }

    void Rigidbody::AddAngularMomentum(const Vector3& velocity)
    {
        Vector3CheckNanException(velocity);
        m_angular_momentum_ += velocity;
    }

    void Rigidbody::AddForce(const Vector3& force)
    {
        m_force_ += force;
    }

    void Rigidbody::AddTorque(const Vector3& torque)
    {
        m_torque_ += torque;
    }

    float Rigidbody::GetFrictionCoefficient() const
    {
        return m_friction_mu_;
    }

    Vector3 Rigidbody::GetLinearMomentum() const
    {
        return m_linear_momentum_;
    }

    Vector3 Rigidbody::GetAngularMomentum() const
    {
        return m_angular_momentum_;
    }

    Vector3 Rigidbody::GetForce() const
    {
        return m_force_;
    }

    Vector3 Rigidbody::GetTorque() const
    {
        return m_torque_;
    }

    bool Rigidbody::GetGrounded() const
    {
        return m_bGrounded;
    }

    bool Rigidbody::IsGravityAllowed() const
    {
        return m_bGravityOverride;
    }

    bool Rigidbody::IsFixed() const
    {
        return m_bFixed;
    }

    bool Rigidbody::IsGrounded() const
    {
        return m_bGrounded;
    }

    void Rigidbody::Reset()
    {
        m_bGrounded = false;

        m_force_            = Vector3::Zero;
        m_torque_           = Vector3::Zero;
        m_drag_force_       = Vector3::Zero;
        m_linear_friction_  = Vector3::Zero;
        m_angular_friction_ = Vector3::Zero;
    }

    void Rigidbody::PreUpdate(const float& dt) {}

    void Rigidbody::Update(const float& dt) {}

    void Rigidbody::PostUpdate(const float& dt)
    {
        Component::PostUpdate(dt);
    }

    void Rigidbody::FixedUpdate(const float& dt) {}

    void Rigidbody::OnDeserialized()
    {
        Component::OnDeserialized();
    }

    void Rigidbody::OnImGui()
    {
        Component::OnImGui();

        ImGui::Indent(2);
        ImGui::Checkbox("Rigidbody Grounded", &m_bGrounded);
        ImGui::Checkbox("Rigidbody Gravity Override", &m_bGravityOverride);
        ImGui::Checkbox("Rigidbody Fixed", &m_bFixed);

        ImGui::DragFloat("Rigidbody Friction", &m_friction_mu_, 0.01f, 0, 1);

        ImGui::Text("Linear Momentum");
        ImGuiVector3Editable(GetID(), "linear_momentum", m_linear_momentum_);
        ImGui::Text("Angular Momentum");
        ImGuiVector3Editable(GetID(), "angular_momentum", m_angular_momentum_);

        ImGui::Text("Linear Friction");
        ImGuiVector3Editable(GetID(), "linear_friction", m_linear_friction_);

        ImGui::Text("Angular Friction");
        ImGuiVector3Editable(GetID(), "angular_friction", m_angular_friction_);
        ImGui::Text("Drag Force");
        ImGuiVector3Editable(GetID(), "drag_force", m_drag_force_);

        ImGui::Text("Rigidbody Force");
        ImGuiVector3Editable(GetID(), "force", m_force_);
        ImGui::Text("Rigidbody Torque");
        ImGuiVector3Editable(GetID(), "torque", m_torque_);

        ImGui::Unindent(2);
    }

    Rigidbody::Rigidbody()
    : Component(COM_T_RIDIGBODY, {}),
      m_bGrounded(false),
      m_bGravityOverride(false),
      m_bFixed(false),
      m_friction_mu_(0) {}
} // namespace Engine::Component
