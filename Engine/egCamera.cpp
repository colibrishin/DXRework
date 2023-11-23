#include "pch.hpp"
#include "egApplication.hpp"
#include "egManagerHelper.hpp"
#include "egCamera.hpp"
#include "egTransform.hpp"

namespace Engine::Objects
{
	void Camera::Initialize()
	{
		AddComponent<Component::Transform>();
		GetComponent<Component::Transform>().lock()->SetPosition({0.0f, 0.0f, -20.0f});
		m_look_at_ = Vector3::Backward;
	}

	void Camera::PreUpdate(const float& dt)
	{
		Object::PreUpdate(dt);
	}

	void Camera::Update(const float& dt)
	{
		Object::Update(dt);

		if (const auto companion = m_bound_object_.lock())
		{
			if (const auto tr_other = companion->GetComponent<Component::Transform>().lock())
			{
				const auto tr = GetComponent<Component::Transform>().lock();
				tr->SetPosition(tr_other->GetPosition() + m_offset_);
			}
		}

		if (GetApplication().GetMouseState().scrollWheelValue > 1)
		{
			GetComponent<Component::Transform>().lock()->Translate(Vector3::Forward * 0.1f);
		}
		else if (GetApplication().GetMouseState().scrollWheelValue < 0)
		{
			GetComponent<Component::Transform>().lock()->Translate(Vector3::Backward * 0.1f);
		}

		const auto current_mouse = GetNormalizedMousePosition();
		Vector2 delta;
		(current_mouse - m_previous_mouse_position_).Normalize(delta);

		m_look_at_ = Vector3::Transform(m_look_at_, Quaternion::CreateFromYawPitchRoll(delta.x * dt, delta.y * dt, 0.f));

		if (const auto transform = GetComponent<Component::Transform>().lock())
		{
			const auto position = transform->GetPosition();
			const auto rotation = transform->GetRotation();

			XMVECTOR upVector = XMLoadFloat3(&Vector3::Up);
			const XMVECTOR positionVector = XMLoadFloat3(&position);
			XMVECTOR lookAtVector = XMLoadFloat3(&m_look_at_);

			// Create the rotation matrix from the yaw, pitch, and roll values.
			const XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(rotation);

			// Transform the lookAt and up vector by the rotation matrix so the view is correctly rotated at the origin.
			lookAtVector = XMVector3TransformCoord(lookAtVector, rotationMatrix);
			upVector = XMVector3TransformCoord(upVector, rotationMatrix);

			// Translate the rotated camera position to the location of the viewer.
			lookAtVector = XMVectorAdd(positionVector, lookAtVector);

			// Finally create the view matrix from the three updated vectors.
			m_view_matrix_ = XMMatrixLookAtLH(positionVector, lookAtVector, upVector);

			const auto p = GetD3Device().GetProjectionMatrix();

			m_vp_buffer_.view = m_view_matrix_.Transpose();
			m_vp_buffer_.projection = p.Transpose();

			GetRenderPipeline().SetPerspectiveMatrix(m_vp_buffer_);
		}
	}

	void Camera::PreRender(const float dt)
	{
		Object::PreRender(dt);
		m_previous_mouse_position_ = GetNormalizedMousePosition();
	}

	void Camera::Render(const float dt)
	{
		Object::Render(dt);
	}

	void Camera::BindObject(const WeakObject& object)
	{
		m_bound_object_ = object;
	}

	void Camera::SetOffset(Vector3 offset)
	{
		m_offset_ = offset;
	}

	Vector2 Camera::GetWorldMousePosition()
	{
		const DirectX::XMMATRIX vp = m_vp_buffer_.projection * m_vp_buffer_.view;
		DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(vp);
		const Vector2 actual_mouse_position
		{
			static_cast<float>(GetApplication().GetMouseState().x),
			static_cast<float>(GetApplication().GetMouseState().y)
		};

		const DirectX::XMMATRIX invProjectionView = DirectX::XMMatrixInverse(&det, vp);

		const float x = (((2.0f * actual_mouse_position.x) / g_window_width) - 1);
		const float y = -(((2.0f * actual_mouse_position.y) / g_window_height) - 1);

		const DirectX::XMVECTOR mousePosition = DirectX::XMVectorSet(x, y, GetComponent<Component::Transform>().lock()->GetPosition().z, 0.0f);

		return DirectX::XMVector3Transform(mousePosition, invProjectionView);
	}

	Vector2 Camera::GetNormalizedMousePosition()
	{
		const Vector2 actual_mouse_position
		{
			static_cast<float>(GetApplication().GetMouseState().x),
			static_cast<float>(GetApplication().GetMouseState().y)
		};

		const float x = (((2.0f * actual_mouse_position.x) / g_window_width) - 1);
		const float y = -(((2.0f * actual_mouse_position.y) / g_window_height) - 1);

		return {x, y};
	}
}
