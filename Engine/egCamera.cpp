#include "pch.h"
#include "egCamera.h"
#include "egApplication.h"
#include "egGlobal.h"
#include "egManagerHelper.hpp"
#include "egTransform.h"
#include "egRigidbody.h"

SERIALIZER_ACCESS_IMPL(
                       Engine::Objects::Camera,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Object))
                       _ARTAG(m_look_at_) _ARTAG(m_offset_)
                       _ARTAG(m_bound_object_id_)
                       _ARTAG(m_b_orthogonal_))

namespace Engine::Objects
{
    void Camera::SetLookAt(Vector3 lookAt)
    {
        m_look_at_ = lookAt;
    }

    void Camera::SetLookAtRotation(Quaternion rotation)
    {
        m_look_at_rotation_ = rotation;
    }

    Quaternion Camera::GetLookAtRotation() const
    {
        return m_look_at_rotation_;
    }

    Matrix Camera::GetViewMatrix() const
    {
        return m_view_matrix_;
    }

    Matrix Camera::GetProjectionMatrix() const
    {
        return m_projection_matrix_;
    }

    Matrix Camera::GetWorldMatrix() const
    {
        return m_world_matrix_;
    }

    Vector3 Camera::GetLookAt() const
    {
        return Vector3::Transform(m_look_at_, m_look_at_rotation_);
    }

    void Camera::Initialize()
    {
        Object::Initialize();

        AddComponent<Components::Transform>();
        GetComponent<Components::Transform>().lock()->SetLocalPosition(
                                                                 {0.0f, 0.0f, -20.0f});
        m_look_at_ = g_forward;
    }

    void Camera::PreUpdate(const float& dt)
    {
        Object::PreUpdate(dt);
    }

    void Camera::Update(const float& dt)
    {
        Object::Update(dt);
    }

    void Camera::PreRender(const float& dt)
    {
        Object::PreRender(dt);

        if (const auto companion = m_bound_object_.lock())
        {
            if (const auto tr_other =
                    companion->GetComponent<Components::Transform>().lock())
            {
                const auto tr = GetComponent<Components::Transform>().lock();
                tr->SetWorldPosition(tr_other->GetWorldPosition() + m_offset_);
                tr->SetWorldRotation(tr_other->GetWorldRotation());
            }
        }

        if (GetApplication().GetMouseState().scrollWheelValue > 1)
        {
            GetComponent<Components::Transform>().lock()->Translate(g_forward * 0.1f);
        }
        else if (GetApplication().GetMouseState().scrollWheelValue < 0)
        {
            GetComponent<Components::Transform>().lock()->Translate(g_backward * 0.1f);
        }

        if (const auto transform = GetComponent<Components::Transform>().lock())
        {
            const auto position = transform->GetWorldPosition();
            const auto rotation = transform->GetWorldRotation();

            Vector3       upVector       = Vector3::Up;
            const Vector3 positionVector = XMLoadFloat3(&position);
            Vector3       lookAtVector   = XMLoadFloat3(&m_look_at_);

            // Create the rotation matrix from the yaw, pitch, and roll values.
            const Matrix rotationMatrix = XMMatrixRotationQuaternion(rotation);
            const Matrix lookAtMatrix   = XMMatrixRotationQuaternion(m_look_at_rotation_);

            m_world_matrix_ =
                    Matrix::CreateWorld(Vector3::Zero, g_forward, Vector3::Up) *
                    rotationMatrix *
                    DirectX::XMMatrixTranslation(position.x, position.y, position.z);

            // Transform the lookAt and up vector by the rotation matrix so the view is
            // correctly rotated at the origin.
            lookAtVector = XMVector3TransformNormal(lookAtVector, rotationMatrix);
            lookAtVector = XMVector3TransformNormal(lookAtVector, lookAtMatrix);
            upVector     = XMVector3TransformNormal(upVector, rotationMatrix);

            // Translate the rotated camera position to the location of the viewer.
            lookAtVector = XMVectorAdd(positionVector, lookAtVector);

            // Finally create the view matrix from the three updated vectors.
            m_view_matrix_ = XMMatrixLookAtLH(positionVector, lookAtVector, upVector);

            m_projection_matrix_ = m_b_orthogonal_
                                       ? GetD3Device().GetOrthogonalMatrix()
                                       : GetD3Device().GetProjectionMatrix();

            const auto invView = m_view_matrix_.Invert();
            const auto invProj = m_projection_matrix_.Invert();

            m_wvp_buffer_.world      = m_world_matrix_.Transpose();
            m_wvp_buffer_.view       = m_view_matrix_.Transpose();
            m_wvp_buffer_.projection = m_projection_matrix_.Transpose();
            m_wvp_buffer_.invView    = invView.Transpose();
            m_wvp_buffer_.invProj    = invProj.Transpose();

            // do the same with 180 degree rotation

            Matrix flipRotation = Matrix::Transform(
                                                    rotationMatrix, Quaternion::CreateFromYawPitchRoll(
                                                     0.f, DirectX::XMConvertToRadians(180.f), 0.f));

            lookAtVector = m_look_at_;

            lookAtVector = XMVector3TransformNormal(lookAtVector, flipRotation);
            lookAtVector = XMVector3TransformNormal(lookAtVector, lookAtMatrix);
            upVector     = XMVector3TransformNormal(upVector, flipRotation);

            lookAtVector              = XMVectorAdd(positionVector, lookAtVector);
            m_wvp_buffer_.reflectView =
                    XMMatrixLookAtLH(positionVector, lookAtVector, upVector);

            m_wvp_buffer_.reflectView = m_wvp_buffer_.reflectView.Transpose();

            GetRenderPipeline().SetPerspectiveMatrix(m_wvp_buffer_);

            Vector3 velocity = Vector3::Zero;

            if (const auto bound = m_bound_object_.lock())
            {
                if (const auto rb = bound->GetComponent<Components::Rigidbody>().lock())
                {
                    velocity = rb->GetLinearMomentum();
                }
            }

            GetToolkitAPI().Set3DListener(
                                          {invView._41, invView._42, invView._43},
                                          {velocity.x, velocity.y, velocity.z},
                                          {g_forward.x, g_forward.y, g_forward.z},
                                          {Vector3::Up.x, Vector3::Up.y, Vector3::Up.z});
        }
    }

    void Camera::Render(const float& dt)
    {
        Object::Render(dt);

#ifdef _DEBUG
        BoundingFrustum frustum;

        BoundingFrustum::CreateFromMatrix(frustum, GetProjectionMatrix());
        frustum.Transform(frustum, GetViewMatrix().Invert());
        GetDebugger().Draw(frustum, DirectX::Colors::WhiteSmoke);
#endif
    }

    void Camera::PostRender(const float& dt)
    {
        Object::PostRender(dt);
    }

    void Camera::FixedUpdate(const float& dt)
    {
        Object::FixedUpdate(dt);
    }

    void Camera::BindObject(const WeakObject& object)
    {
        m_bound_object_    = object;
        m_bound_object_id_ = object.lock()->GetLocalID();
    }

    void Camera::SetOffset(Vector3 offset)
    {
        m_offset_ = offset;
    }

    void Camera::SetOrthogonal(bool bOrthogonal)
    {
        m_b_orthogonal_ = bOrthogonal;
    }

    bool Camera::GetOrthogonal() const
    {
        return m_b_orthogonal_;
    }

    Vector2 Camera::GetWorldMousePosition()
    {
        const Matrix  pv = GetD3Device().GetProjectionMatrix() * m_view_matrix_;
        const Vector2 actual_mouse_position{
            static_cast<float>(GetApplication().GetMouseState().x),
            static_cast<float>(GetApplication().GetMouseState().y)
        };

        const Matrix invProjectionView = XMMatrixInverse(nullptr, pv);

        const float x = (((2.0f * actual_mouse_position.x) / g_window_width) - 1);
        const float y = -(((2.0f * actual_mouse_position.y) / g_window_height) - 1);

        const Vector3 mousePosition = DirectX::XMVectorSet(
                                                           x, y, GetComponent<Components::Transform>().lock()->
                                                           GetWorldPosition().z, 0.0f);

        return XMVector3Transform(mousePosition, invProjectionView);
    }

    void Camera::OnDeserialized()
    {
        Object::OnDeserialized();
    }

    void Camera::OnImGui()
    {
        Object::OnImGui();

        if (ImGui::Begin(
                         "Camera", nullptr,
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoCollapse))
        {
            ImGui::Text("Look at");
            ImGuiVector3Editable(GetID(), "look_at", m_look_at_);

            ImGui::Text("Offset");
            ImGuiVector3Editable(GetID(), "offset", m_offset_);

            ImGui::Text("Orthogonal");
            ImGui::Checkbox("##orthogonal", &m_b_orthogonal_);

            ImGui::Text("Bound object: %lld", m_bound_object_id_);
            ImGui::End();
        }
    }
} // namespace Engine::Objects
