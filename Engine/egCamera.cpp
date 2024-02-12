#include "pch.h"
#include "egCamera.h"
#include "egApplication.h"
#include "egGlobal.h"
#include "egManagerHelper.hpp"
#include "egRigidbody.h"
#include "egTransform.h"

SERIALIZER_ACCESS_IMPL
(
 Engine::Objects::Camera,
 _ARTAG(_BSTSUPER(Engine::Abstract::Object))
 _ARTAG(m_b_orthogonal_)
)

namespace Engine::Objects
{
  Matrix Camera::GetViewMatrix() const { return m_view_matrix_; }

  Matrix Camera::GetProjectionMatrix() const { return m_projection_matrix_; }

  Matrix Camera::GetWorldMatrix() const { return m_world_matrix_; }

  void Camera::Initialize()
  {
    Object::Initialize();

    AddComponent<Components::Transform>();
  }

  void Camera::PreUpdate(const float& dt) { Object::PreUpdate(dt); }

  void Camera::Update(const float& dt) { Object::Update(dt); }

  void Camera::PreRender(const float& dt)
  {
    Object::PreRender(dt);
    m_previous_view_matrix_ = m_view_matrix_;

    if (GetApplication().GetMouseState().scrollWheelValue > 1)
    {
      GetComponent<Components::Transform>().lock()->Translate
        (g_forward * 0.1f);
    }
    else if (GetApplication().GetMouseState().scrollWheelValue < 0)
    {
      GetComponent<Components::Transform>().lock()->
                                            Translate(g_backward * 0.1f);
    }

    if (const auto transform = GetComponent<Components::Transform>().lock())
    {
      const auto position = transform->GetWorldPosition();
      const auto rotation = transform->GetWorldRotation();
      Vector3    up       = m_b_fixed_up_ ? Vector3::Up : transform->Up();
      Vector3    forward  = transform->Forward();

      Matrix rotationMatrix = Matrix::CreateFromQuaternion(rotation);

      m_world_matrix_ =
        Matrix::CreateWorld(Vector3::Zero, g_forward, Vector3::Up) *
        rotationMatrix *
        Matrix::CreateTranslation(position);

      // Finally create the view matrix from the three updated vectors.
      m_view_matrix_ = XMMatrixLookAtLH
        (
         position,
         position + forward,
         up
        );

      if (m_b_orthogonal_)
      {
        float aspectRatio = static_cast<float>(g_window_width) / static_cast<float>(g_window_height);

        m_projection_matrix_ = DirectX::XMMatrixOrthographicLH
          (
           m_zoom_ * aspectRatio, aspectRatio, g_screen_near, g_screen_far
          );
      }
      else
      {
        m_projection_matrix_ = GetD3Device().GetProjectionMatrix();
      }

      const auto invView = m_view_matrix_.Invert();
      const auto invProj = m_projection_matrix_.Invert();

      m_wvp_buffer_.world      = m_world_matrix_.Transpose();
      m_wvp_buffer_.view       = m_view_matrix_.Transpose();
      m_wvp_buffer_.projection = m_projection_matrix_.Transpose();
      m_wvp_buffer_.invView    = invView.Transpose();
      m_wvp_buffer_.invProj    = invProj.Transpose();
      m_wvp_buffer_.prevView   = m_previous_view_matrix_.Transpose();
      m_wvp_buffer_.prevProj   = m_previous_projection_matrix_.Transpose();

      // do the same with 180 degree rotation

      Matrix flipRotation = Matrix::Transform
        (
         rotationMatrix, Quaternion::CreateFromYawPitchRoll
         (
          0.f, DirectX::XMConvertToRadians(180.f), 0.f
         )
        );

      Vector3 flipLookAtVector = XMVector3TransformNormal(forward, flipRotation);
      Vector3 flipUpVector     = XMVector3TransformNormal(up, flipRotation);

      m_wvp_buffer_.reflectView = XMMatrixLookAtLH
        (
         position,
         position + flipLookAtVector,
         flipUpVector
        );

      m_wvp_buffer_.reflectView = m_wvp_buffer_.reflectView.Transpose();

      GetRenderPipeline().SetPerspectiveMatrix(m_wvp_buffer_);

      Vector3 velocity = Vector3::Zero;

      if (const auto bound = GetParent().lock())
      {
        if (const auto rb = bound->GetComponent<Components::Rigidbody>().lock()) { velocity = rb->GetT0LinearVelocity(); }
      }

      GetToolkitAPI().Set3DListener
        (
         {invView._41, invView._42, invView._43},
         {velocity.x, velocity.y, velocity.z},
         {g_forward.x, g_forward.y, g_forward.z},
         {Vector3::Up.x, Vector3::Up.y, Vector3::Up.z}
        ); // todo: fixed up?
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

  void Camera::PostRender(const float& dt) { Object::PostRender(dt); }

  void Camera::FixedUpdate(const float& dt) { Object::FixedUpdate(dt); }

  void Camera::SetOrthogonal(bool bOrthogonal) { m_b_orthogonal_ = bOrthogonal; }

  void Camera::SetFixedUp(bool bFixedUp) { m_b_fixed_up_ = bFixedUp; }

  void Camera::SetZoom(float zoom)
  {
    m_zoom_ = zoom;
  }

  bool Camera::GetOrthogonal() const { return m_b_orthogonal_; }

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

    const Vector3 mousePosition = DirectX::XMVectorSet
      (
       x, y, GetComponent<Components::Transform>().lock()->
                                                   GetWorldPosition().z, 0.0f
      );

    return XMVector3Transform(mousePosition, invProjectionView);
  }

  void Camera::OnDeserialized() { Object::OnDeserialized(); }

  void Camera::OnImGui()
  {
    Object::OnImGui();

    if (ImGui::Begin
      (
       "Camera", nullptr,
       ImGuiWindowFlags_AlwaysAutoResize |
       ImGuiWindowFlags_NoCollapse
      ))
    {
      ImGui::Text("Orthogonal");
      ImGui::Checkbox("##orthogonal", &m_b_orthogonal_);
      ImGui::End();
    }
  }
} // namespace Engine::Objects
