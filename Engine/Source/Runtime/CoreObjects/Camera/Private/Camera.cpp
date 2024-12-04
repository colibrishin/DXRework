#include "../Public/Camera.h"
#include "Source/Runtime/Components/Transform/Public/Transform.h"
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#include "Source/Runtime/Components/Rigidbody/Public/Rigidbody.h"
#include "Source/Runtime/Managers/SoundMananger/Public/SoundManager.h"

SERIALIZE_IMPL
(
 Engine::Objects::Camera,
 _ARTAG(_BSTSUPER(Engine::Abstracts::ObjectBase))
 _ARTAG(m_b_orthogonal_)
)

namespace Engine::Objects
{
	OBJ_CLONE_IMPL(Camera)

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

	void Camera::Initialize()
	{
		ObjectBase::Initialize();

		AddComponent<Components::Transform>();
	}

	void Camera::PreUpdate(const float& dt)
	{
		ObjectBase::PreUpdate(dt);
	}

	void Camera::Update(const float& dt)
	{
		ObjectBase::Update(dt);
	}

	void Camera::PreRender(const float& dt)
	{
		ObjectBase::PreRender(dt);

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
					 position - forward,
					 up
					);

			if (m_b_orthogonal_)
			{
				float aspectRatio = static_cast<float>(CFG_WIDTH) / static_cast<float>(CFG_HEIGHT);

				m_projection_matrix_ = DirectX::XMMatrixOrthographicLH
						(
						 m_fov_ * aspectRatio, m_fov_, CFG_SCREEN_NEAR, CFG_SCREEN_FAR
						);
			}
			else
			{
				m_projection_matrix_ = Managers::D3Device::GetInstance().GetProjectionMatrix();
			}

			const auto invView = m_view_matrix_.Invert();
			const auto invProj = m_projection_matrix_.Invert();

			m_wvp_buffer_.world = m_world_matrix_.Transpose();
			m_wvp_buffer_.view = m_view_matrix_.Transpose();
			m_wvp_buffer_.projection = m_projection_matrix_.Transpose();
			m_wvp_buffer_.invView = invView.Transpose();
			m_wvp_buffer_.invProj = invProj.Transpose();
			m_wvp_buffer_.invVP = XMMatrixTranspose(XMMatrixInverse(nullptr, m_view_matrix_ * m_projection_matrix_));

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

			// todo: only main camera should do this behaviour.
#if CFG_RAYTRACING
			GetRaytracingPipeline().SetPerspectiveMatrix(m_wvp_buffer_);
#else
			Managers::RenderPipeline::GetInstance().SetPerspectiveMatrix(m_wvp_buffer_);
#endif

			Vector3 velocity = Vector3::Zero;

			if (const auto bound = GetParent().lock())
			{
				if (const auto rb = bound->GetComponent<Components::Rigidbody>().lock())
				{
					velocity = rb->GetT0LinearVelocity();
				}
			}

			Managers::SoundManager::GetInstance().Set3DListener
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
		ObjectBase::Render(dt);

#if WITH_DEBUG
		BoundingFrustum frustum;

		BoundingFrustum::CreateFromMatrix(frustum, GetProjectionMatrix());
		frustum.Transform(frustum, GetViewMatrix().Invert());
		GetDebugger().Draw(frustum, DirectX::Colors::WhiteSmoke);
#endif
	}

	void Camera::PostRender(const float& dt)
	{
		ObjectBase::PostRender(dt);
	}

	void Camera::FixedUpdate(const float& dt)
	{
		ObjectBase::FixedUpdate(dt);
	}

	void Camera::SetOrthogonal(bool bOrthogonal)
	{
		m_b_orthogonal_ = bOrthogonal;
	}

	void Camera::SetFixedUp(bool bFixedUp)
	{
		m_b_fixed_up_ = bFixedUp;
	}

	void Camera::SetFOV(float zoom)
	{
		m_fov_ = zoom;
	}

	bool Camera::GetOrthogonal() const
	{
		return m_b_orthogonal_;
	}

	float Camera::GetFOV() const
	{
		return m_fov_;
	}

	Vector2 Camera::GetWorldMousePosition()
	{
		const Matrix  pv = Managers::D3Device::GetInstance().GetProjectionMatrix() * m_view_matrix_;
		const Vector2 actual_mouse_position{
			static_cast<float>(Managers::InputManager::GetInstance().GetMouseState().x),
			static_cast<float>(Managers::InputManager::GetInstance().GetMouseState().y)
		};

		const Matrix invProjectionView = XMMatrixInverse(nullptr, pv);

		const float x = (((2.0f * actual_mouse_position.x) / CFG_WIDTH) - 1);
		const float y = -(((2.0f * actual_mouse_position.y) / CFG_HEIGHT) - 1);

		const Vector3 mousePosition = DirectX::XMVectorSet
				(
				 x, y, GetComponent<Components::Transform>().lock()->
				                                             GetWorldPosition().z, 0.0f
				);

		return XMVector3Transform(mousePosition, invProjectionView);
	}

	void Camera::OnDeserialized()
	{
		ObjectBase::OnDeserialized();
	}
} // namespace Engine::Objects
