#include "pch.hpp"

#include "egManagerHelper.hpp"
#include "egCamera.hpp"
#include "egTransform.hpp"

namespace Engine::Objects
{
	void Camera::Initialize()
	{
		AddComponent<Component::Transform>();
		GetComponent<Component::Transform>().lock()->SetPosition({0.0f, 0.0f, -10.0f});
		m_look_at_ = Vector3::Backward;
	}

	void Camera::PreUpdate(const float& dt)
	{
		Object::PreUpdate(dt);
	}

	void Camera::Update(const float& dt)
	{
		Object::Update(dt);

		if (GetApplication().GetMouseState().scrollWheelValue > 1)
		{
			GetComponent<Component::Transform>().lock()->Translate(Vector3::Forward * 0.1f);
		}
		else if (GetApplication().GetMouseState().scrollWheelValue < 0)
		{
			GetComponent<Component::Transform>().lock()->Translate(Vector3::Backward * 0.1f);
		}

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
	}

	void Camera::Render(const float dt)
	{
		Object::Render(dt);
	}
}
