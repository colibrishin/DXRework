#include "pch.h"
#include "egTransform.h"

#include "egManagerHelper.hpp"

SERIALIZER_ACCESS_IMPL(
                       Engine::Components::Transform,
                       _ARTAG(_BSTSUPER(Component)) _ARTAG(m_previous_position_)
                       _ARTAG(m_position_) _ARTAG(m_rotation_)
                       _ARTAG(m_scale_))

namespace Engine::Components
{
    Transform::Transform(const WeakObject& owner)
    : Component(COM_T_TRANSFORM, owner),
      m_b_absolute_(true),
      m_previous_position_(Vector3::Zero),
      m_position_(Vector3::Zero),
      m_rotation_(Quaternion::Identity),
      m_scale_(Vector3::One) {}

    void Transform::SetYawPitchRoll(const Vector3& yaw_pitch_roll)
    {
        m_yaw_pitch_roll_degree_ = yaw_pitch_roll;
        m_rotation_              = Quaternion::CreateFromYawPitchRoll(
                                                         DirectX::XMConvertToRadians(yaw_pitch_roll.y),
                                                         DirectX::XMConvertToRadians(yaw_pitch_roll.x),
                                                         DirectX::XMConvertToRadians(yaw_pitch_roll.z));
    }

    void Transform::SetWorldPosition(const Vector3& position)
    {
        Matrix world = GetWorldMatrix();
        Matrix inv_local = GetLocalMatrix().Invert();
        world = inv_local * world;
        world = world.Invert();

        m_position_ = Vector3::Transform(position, world);
    }

    void Transform::SetWorldRotation(const Quaternion& rotation)
    {
        Quaternion world = GetWorldRotation();
        Quaternion inv_local;
        GetLocalRotation().Inverse(inv_local);
        world = inv_local * world;
        world.Inverse(world);

        m_rotation_ = rotation * world;
    }

    void Transform::SetLocalPosition(const Vector3& position)
    {
        m_position_ = position;
    }

    void Transform::SetLocalRotation(const Quaternion& rotation)
    {
        m_rotation_ = rotation;
        const auto euler = rotation.ToEuler();

        m_yaw_pitch_roll_degree_ = Vector3{
            XMConvertToDegrees(euler.x), XMConvertToDegrees(euler.y), XMConvertToDegrees(euler.z)
        };
    }

    void Transform::SetScale(const Vector3& scale)
    {
        m_scale_ = scale;
    }

    void Transform::SetSizeAbsolute(bool absolute)
    {
        m_b_absolute_ = absolute;
    }

    Vector3 Transform::GetWorldPosition() const
    {
        Matrix              world = GetWorldMatrix();
        return {world._41, world._42, world._43};
    }

    Quaternion Transform::GetWorldRotation() const
    {
        Quaternion rotation = m_rotation_;
        const WeakTransform tr = FindNextTransform(*this);

        if (const auto parent = tr.lock())
        {
            rotation *= parent->GetWorldRotation();
        }

        return rotation;
    }

    Vector3 Transform::GetLocalPosition() const
    {
        return m_position_;
    }

    Quaternion Transform::GetLocalRotation() const
    {
        return m_rotation_;
    }

    Vector3 Transform::GetScale() const
    {
        return m_scale_;
    }

    Vector3 Transform::Forward() const
    {
        return Vector3::Transform(g_forward, m_rotation_);
    }

    Vector3 Transform::Right() const
    {
        return Vector3::Transform(Vector3::Right, m_rotation_);
    }

    Vector3 Transform::Up() const
    {
        return Vector3::Transform(Vector3::Up, m_rotation_);
    }

    void Transform::Translate(Vector3 translation)
    {
        m_position_ += translation;
    }

    void Transform::Initialize()
    {
        Component::Initialize();
        m_world_previous_position_ = GetWorldPosition();
        m_previous_position_ = m_position_;
    }

    void Transform::PreUpdate(const float& dt)
    {
        m_rotation_ = Quaternion::CreateFromYawPitchRoll(
                                                         XMConvertToRadians(m_yaw_pitch_roll_degree_.x),
                                                         XMConvertToRadians(m_yaw_pitch_roll_degree_.y),
                                                         XMConvertToRadians(m_yaw_pitch_roll_degree_.z));
    }

    void Transform::Update(const float& dt) {}

    void Transform::PreRender(const float& dt) {}

    void Transform::PostRender(const float& dt)
    {
        m_world_previous_position_ = GetWorldPosition();
        m_previous_position_ = m_position_;
    }

    void Transform::Render(const float& dt)
    {
        Component::Render(dt);
        m_transform_buffer_.world = GetWorldMatrix().Transpose();
        GetRenderPipeline().SetWorldMatrix(m_transform_buffer_);
    }

    void Transform::FixedUpdate(const float& dt) {}

    void Transform::OnImGui()
    {
        Component::OnImGui();
        ImGui::Indent(2);

        ImGui::Text("Previous Position");
        ImGuiVector3Editable(GetID(), "previous_position", m_previous_position_);

        ImGui::Text("Position");
        ImGuiVector3Editable(GetID(), "position", m_position_);

        ImGui::Text("Rotation");
        ImGuiVector3Editable(GetID(), "yaw_pitch_roll", m_yaw_pitch_roll_degree_);

        ImGui::Text("Scale");
        ImGuiVector3Editable(GetID(), "scale", m_scale_);
        ImGui::Unindent(2);
    }

    void Transform::OnDeserialized()
    {
        Component::OnDeserialized();
        const auto euler = m_rotation_.ToEuler();

        m_yaw_pitch_roll_degree_ =
                Vector3(
                        XMConvertToDegrees(euler.y), XMConvertToDegrees(euler.x),
                        XMConvertToDegrees(euler.z));
    }

    Matrix Transform::GetLocalMatrix() const
    {
        return Matrix::CreateScale(m_scale_) *
               Matrix::CreateFromQuaternion(m_rotation_) *
               Matrix::CreateTranslation(m_position_);
    }

    Vector3 Transform::GetWorldPreviousPosition() const
    {
        return m_world_previous_position_;
    }

    Vector3 Transform::GetLocalPreviousPosition() const
    {
        return m_previous_position_;
    }

    Matrix Transform::GetWorldMatrix() const
    {
        Matrix              world = GetLocalMatrix();
        const WeakTransform tr    = FindNextTransform(*this);

        if (const auto parent = tr.lock())
        {
            if (m_b_absolute_)
            {
                const auto inv_scale = Matrix::CreateScale(parent->GetScale()).Invert();
                world *= inv_scale * parent->GetWorldMatrix();
            }
            else
            {
                world *= parent->GetWorldMatrix();
            }
        }

        return world;
    }

    Transform::Transform()
    : Component(COM_T_TRANSFORM, {}),
      m_b_absolute_(true),
      m_previous_position_(Vector3::Zero),
      m_position_(Vector3::Zero),
      m_rotation_(Quaternion::Identity),
      m_scale_(Vector3::One) {}

    WeakTransform Transform::FindNextTransform(const Transform& transform_)
    {
        WeakObject next = transform_.GetOwner().lock()->GetParent();

        while (const auto parent = next.lock())
        {
            if (const auto transform = parent->GetComponent<Transform>().lock())
            {
                return transform;
            }

            next = parent->GetParent();
        }

        return {};
    }
} // namespace Engine::Component
