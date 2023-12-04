#include "pch.hpp"
#include "egApplication.hpp"
#include "egManagerHelper.hpp"
#include "egCamera.hpp"
#include "egTransform.hpp"

SERIALIZER_ACCESS_IMPL(
	Engine::Objects::Camera,
	_ARTAG(_BSTSUPER(Engine::Abstract::Object))
	_ARTAG(m_look_at_)
	_ARTAG(m_mouse_rotation_)
	_ARTAG(m_previous_mouse_position_)
	_ARTAG(m_current_mouse_position_)
	_ARTAG(m_offset_)
	_ARTAG(m_bound_object_id_)
	_ARTAG(m_b_orthogonal_))

namespace Engine::Objects
{
	void Camera::SetPosition(Vector3 position)
	{
		GetComponent<Component::Transform>().lock()->SetPosition(position);
	}

	void Camera::SetRotation(Quaternion rotation)
	{
		GetComponent<Component::Transform>().lock()->SetRotation(rotation);
	}

	Vector3 Camera::GetPosition()
	{
		return GetComponent<Component::Transform>().lock()->GetPosition();
	}

	Vector3 Camera::GetLookAt() const
	{
		return Vector3::Transform(m_look_at_, m_mouse_rotation_matrix_);
	}

	void Camera::Initialize()
	{
		Object::Initialize();

		AddComponent<Component::Transform>();
		GetComponent<Component::Transform>().lock()->SetPosition({0.0f, 0.0f, -20.0f});
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

	void Camera::PreRender(const float dt)
	{
		Object::PreRender(dt);

		if (const auto companion = m_bound_object_.lock())
		{
			if (const auto tr_other = companion->GetComponent<Component::Transform>().lock())
			{
				const auto tr = GetComponent<Component::Transform>().lock();
				tr->SetPosition(tr_other->GetPosition() + m_offset_);
				tr->SetRotation(tr_other->GetRotation());
			}
		}

		if (GetApplication().GetMouseState().scrollWheelValue > 1)
		{
			GetComponent<Component::Transform>().lock()->Translate(g_forward * 0.1f);
		}
		else if (GetApplication().GetMouseState().scrollWheelValue < 0)
		{
			GetComponent<Component::Transform>().lock()->Translate(g_backward * 0.1f);
		}

		m_current_mouse_position_ = GetNormalizedMousePosition();
		Vector2 delta;
		(m_current_mouse_position_ - m_previous_mouse_position_).Normalize(delta);

		const auto lookRotation = Quaternion::CreateFromYawPitchRoll(delta.x * dt, delta.y * dt, 0.f);
		m_mouse_rotation_ = Quaternion::Concatenate(m_mouse_rotation_, lookRotation);
		m_mouse_rotation_matrix_ = Matrix::CreateFromQuaternion(m_mouse_rotation_);

		if (const auto transform = GetComponent<Component::Transform>().lock())
		{
			const auto position = transform->GetPosition();
			const auto rotation = transform->GetRotation();

			Vector3 upVector = Vector3::Up;
			const XMVECTOR positionVector = XMLoadFloat3(&position);
			Vector3 lookAtVector = XMLoadFloat3(&m_look_at_);

			// Create the rotation matrix from the yaw, pitch, and roll values.
			const XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(rotation);

			// Transform the lookAt and up vector by the rotation matrix so the view is correctly rotated at the origin.
			lookAtVector = XMVector3TransformNormal(lookAtVector, rotationMatrix);
			upVector = XMVector3TransformNormal(upVector, rotationMatrix);
			lookAtVector = XMVector3TransformNormal(lookAtVector, m_mouse_rotation_matrix_);

			// Translate the rotated camera position to the location of the viewer.
			lookAtVector = XMVectorAdd(positionVector, lookAtVector);

			// Finally create the view matrix from the three updated vectors.
			m_view_matrix_ = XMMatrixLookAtLH(positionVector, lookAtVector, upVector);

			const auto p = m_b_orthogonal_ ? GetD3Device().GetOrthogonalMatrix() : GetD3Device().GetProjectionMatrix();

			m_vp_buffer_.view = m_view_matrix_.Transpose();
			m_vp_buffer_.projection = p.Transpose();

			GetRenderPipeline().SetPerspectiveMatrix(m_vp_buffer_);

			Vector3 velocity = Vector3::Zero;

			if (const auto bound = m_bound_object_.lock())
			{
				if (const auto rb = bound->GetComponent<Component::Rigidbody>().lock())
				{
					velocity = rb->GetLinearMomentum();
				}
			}

			GetToolkitAPI().Set3DListener(
				{m_view_matrix_._41, m_view_matrix_._42, m_view_matrix_._43},
				{velocity.x, velocity.y, velocity.z},
				{g_forward.x, g_forward.y, g_forward.z},
				{Vector3::Up.x, Vector3::Up.y, Vector3::Up.z}
			);
		}
	}

	void Camera::Render(const float dt)
	{
		Object::Render(dt);
		m_previous_mouse_position_ = m_current_mouse_position_;
	}

	void Camera::FixedUpdate(const float& dt)
	{
		Object::FixedUpdate(dt);
	}

	void Camera::BindObject(const WeakObject& object)
	{
		m_bound_object_ = object;
		m_bound_object_id_ = object.lock()->GetLocalID();
	}

	void Camera::SetOffset(Vector3 offset)
	{
		m_offset_ = offset;
	}

	Vector2 Camera::GetWorldMousePosition()
	{
		const DirectX::XMMATRIX vp = GetD3Device().GetProjectionMatrix() * m_view_matrix_;
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

	void Camera::OnDeserialized()
	{
		Object::OnDeserialized();
	}
}
