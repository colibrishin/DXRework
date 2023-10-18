#include "egCamera.hpp"
#include "pch.hpp"
#include "egTransform.hpp"

namespace Engine::Objects
{
	void Camera::Initialize()
	{
		AddComponent<Component::Transform>();
		GetComponent<Component::Transform>().lock()->SetPosition({0.0f, 0.0f, -5.0f});
		m_rotation_ = XMQuaternionIdentity();
		m_look_at_ = Vector3::Backward;
	}

	void Camera::PreUpdate()
	{
		Object::PreUpdate();
	}

	void Camera::Update()
	{
		Object::Update();
		if (const auto transform = GetComponent<Component::Transform>().lock())
		{
			const auto position = transform->GetPosition();

			XMVECTOR upVector = XMLoadFloat3(&Vector3::Up);
			const XMVECTOR positionVector = XMLoadFloat3(&position);
			XMVECTOR lookAtVector = XMLoadFloat3(&m_look_at_);

			// Create the rotation matrix from the yaw, pitch, and roll values.
			const XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(m_rotation_);

			// Transform the lookAt and up vector by the rotation matrix so the view is correctly rotated at the origin.
			lookAtVector = XMVector3TransformCoord(lookAtVector, rotationMatrix);
			upVector = XMVector3TransformCoord(upVector, rotationMatrix);

			// Translate the rotated camera position to the location of the viewer.
			lookAtVector = XMVectorAdd(positionVector, lookAtVector);

			// Finally create the view matrix from the three updated vectors.
			m_view_matrix_ = XMMatrixLookAtLH(positionVector, lookAtVector, upVector);

			const auto w = Graphic::D3Device::GetWorldMatrix();
			const auto p = Graphic::D3Device::GetProjectionMatrix();

			m_vp_buffer_.view = m_view_matrix_.Transpose();
			m_vp_buffer_.projection = p.Transpose();

			Graphic::RenderPipeline::SetPerspectiveMatrix(m_vp_buffer_);
		}
	}

	void Camera::PreRender()
	{
		Object::PreRender();
	}

	void Camera::Render()
	{
		Object::Render();
	}
}
