#include "pch.h"
#include "egTransform.h"

#include "egManagerHelper.hpp"

SERIALIZER_ACCESS_IMPL(Engine::Components::Transform,
                          _ARTAG(_BSTSUPER(Component))
                          _ARTAG(m_b_absolute_)
                          _ARTAG(m_previous_position_)
                          _ARTAG(m_position_)
                          _ARTAG(m_rotation_)
                          _ARTAG(m_scale_)
                          _ARTAG(m_animation_position_)
                          _ARTAG(m_animation_rotation_)
                          _ARTAG(m_animation_scale_))

namespace Engine::Components
{
    Transform::Transform(const WeakObject& owner)
    : Component(COM_T_TRANSFORM, owner),
      m_b_absolute_(true),
      m_previous_position_(Vector3::Zero),
      m_position_(Vector3::Zero),
      m_rotation_(Quaternion::Identity),
      m_scale_(Vector3::One),
      m_animation_position_(Vector3::Zero),
      m_animation_rotation_(Quaternion::Identity),
      m_animation_scale_(Vector3::One),
      m_b_lazy_(true) {}

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
        m_b_lazy_   = true;
    }

    void Transform::SetWorldRotation(const Quaternion& rotation)
    {
        Quaternion world = GetWorldRotation();
        Quaternion inv_local;
        GetLocalRotation().Inverse(inv_local);
        world = inv_local * world;
        world.Inverse(world);

        m_rotation_ = rotation * world;
        m_b_lazy_   = true;
    }

    void Transform::SetLocalPosition(const Vector3& position)
    {
        m_position_ = position;
        m_b_lazy_   = true;
    }

    void Transform::SetLocalRotation(const Quaternion& rotation)
    {
        m_rotation_ = rotation;
        const auto euler = rotation.ToEuler();

        m_yaw_pitch_roll_degree_ = Vector3{
            DirectX::XMConvertToDegrees(euler.x), DirectX::XMConvertToDegrees(euler.y), DirectX::XMConvertToDegrees(euler.z)
        };
        m_b_lazy_ = true;
    }

    void Transform::SetScale(const Vector3& scale)
    {
        m_scale_ = scale;
        m_b_lazy_ = true;
    }

    void Transform::SetSizeAbsolute(bool absolute)
    {
        m_b_absolute_ = absolute;
        m_b_lazy_     = true;
    }

    void Transform::SetAnimationPosition(const Vector3& position)
    {
        m_animation_position_ = position;
        m_b_lazy_             = true;
    }

    void Transform::SetAnimationRotation(const Quaternion& rotation)
    {
        m_animation_rotation_ = rotation;
        m_b_lazy_             = true;
    }

    void Transform::SetAnimationScale(const Vector3& scale)
    {
        m_animation_scale_ = scale;
        m_b_lazy_          = true;
    }

    Vector3 Transform::GetWorldPosition()
    {
        Matrix              world = GetWorldMatrix();
        return {world._41, world._42, world._43};
    }

    Quaternion Transform::GetWorldRotation() const
    {
        Quaternion rotation = GetLocalRotation();
        const WeakTransform tr = FindNextTransform(*this);

        if (m_b_absolute_)
        {
            return rotation;
        }

        if (const auto parent = tr.lock())
        {
            rotation = Quaternion::Concatenate(rotation, parent->GetWorldRotation());
        }

        return rotation;
    }

    Vector3 Transform::GetLocalPosition() const
    {
        return m_position_ + m_animation_position_;
    }

    Quaternion Transform::GetLocalRotation() const
    {
        return Quaternion::Concatenate(m_rotation_, m_animation_rotation_);
    }

    Vector3 Transform::GetScale() const
    {
        return m_scale_ * m_animation_scale_;
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
        m_b_lazy_             = true;
    }

    void Transform::Initialize()
    {
        Component::Initialize();
        m_world_previous_position_ = GetWorldPosition();
        m_previous_position_ = GetLocalPosition();
    }

    void Transform::PreUpdate(const float& dt)
    {
        m_rotation_ = Quaternion::CreateFromYawPitchRoll(
                                                         DirectX::XMConvertToRadians(m_yaw_pitch_roll_degree_.x),
                                                         DirectX::XMConvertToRadians(m_yaw_pitch_roll_degree_.y),
                                                         DirectX::XMConvertToRadians(m_yaw_pitch_roll_degree_.z));
    }

    void Transform::Update(const float& dt) {}

    void Transform::PostUpdate(const float& dt)
    {
        Component::PostUpdate(dt);
        m_world_previous_position_ = GetWorldPosition();
        m_previous_position_ = GetLocalPosition();
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
                        DirectX::XMConvertToDegrees(euler.y), DirectX::XMConvertToDegrees(euler.x),
                        DirectX::XMConvertToDegrees(euler.z));
    }

    Matrix Transform::GetLocalMatrix() const
    {
        return Matrix::CreateScale(GetScale()) *
               Matrix::CreateFromQuaternion(GetLocalRotation()) *
               Matrix::CreateTranslation(GetLocalPosition());
    }

    Vector3 Transform::GetWorldPreviousPosition() const
    {
        return m_world_previous_position_;
    }

    Vector3 Transform::GetLocalPreviousPosition() const
    {
        return m_previous_position_;
    }

    Matrix Transform::GetWorldMatrix()
    {
        if (!m_b_lazy_)
        {
            return m_world_matrix_;
        }

        Matrix        world = GetLocalMatrix();
        auto          tr_c  = const_cast<Transform*>(this);
        WeakTransform tr_p  = FindNextTransform(*this);

        if (const auto parent = tr_p.lock())
        {
            if (tr_c->m_b_absolute_)
            {
                const auto inv_sr = Matrix::CreateScale(parent->GetScale()).Invert() * 
                    Matrix::CreateFromQuaternion(parent->GetLocalRotation()).Invert();
                world *= inv_sr * parent->GetLocalMatrix();
            }
            else
            {
                world *= parent->GetLocalMatrix();
            }

            tr_c = parent.get();
            tr_p = FindNextTransform(*parent);
        }

        m_b_lazy_       = false;
        m_world_matrix_ = world;

        return world;
    }

    void Transform::Bind(Transform& transform)
    {
        transform.m_transform_buffer_.world = transform.GetWorldMatrix().Transpose();
        GetRenderPipeline().SetWorldMatrix(transform.m_transform_buffer_);
    }

    void Transform::Unbind()
    {
        GetRenderPipeline().SetWorldMatrix({});
    }

    Transform::Transform()
    : Component(COM_T_TRANSFORM, {}),
      m_b_absolute_(true),
      m_previous_position_(Vector3::Zero),
      m_position_(Vector3::Zero),
      m_rotation_(Quaternion::Identity),
      m_scale_(Vector3::One),
      m_animation_position_(Vector3::Zero),
      m_animation_rotation_(Quaternion::Identity),
      m_animation_scale_(Vector3::One),
      m_b_lazy_(true) {}

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
