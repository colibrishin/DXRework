#include "../Public/CameraManager.h"

#include "Source/Runtime/Core/Components/Rigidbody/Public/Rigidbody.h"
#include "Source/Runtime/Core/Components/Transform/Public/Transform.h"
#include "Source/Runtime/Core/Objects/Camera/Public/Camera.h"

#include "Source/Runtime/Core/SceneManager/Public/SceneManager.hpp"
#include "Source/Runtime/Managers/D3D12Wrapper/Public/D3Device.hpp"
#include "Source/Runtime/Managers/InputManager/Public/InputManager.h"

#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#include "Source/Runtime/Managers/SoundMananger/Public/SoundManager.h"

namespace Engine::Managers
{
	void CameraManager::Initialize() {}

	void CameraManager::PreUpdate(const float& dt) {}

	void CameraManager::Update(const float& dt) {}

	void CameraManager::PreRender(const float& dt) 
	{
		if (const Strong<Scene>& scene = Managers::SceneManager::GetInstance().GetActiveScene().lock()) 
		{
			if (const Strong<Objects::Camera>& camera = scene->GetMainCamera().lock()) 
			{
				if (const auto transform = camera->GetComponent<Components::Transform>().lock())
				{
					const auto position = transform->GetWorldPosition();
					const auto rotation = transform->GetWorldRotation();
					Vector3    up       = camera->m_b_fixed_up_ ? Vector3::Up : transform->Up();
					Vector3    forward  = transform->Forward();

					Matrix rotationMatrix = Matrix::CreateFromQuaternion(rotation);

					camera->m_world_matrix_ =
							Matrix::CreateWorld(Vector3::Zero, g_forward, Vector3::Up) *
							rotationMatrix *
							Matrix::CreateTranslation(position);

					// Finally create the view matrix from the three updated vectors.
					camera->m_view_matrix_ = XMMatrixLookAtLH
							(
							position,
							position - forward,
							up
							);

					if (camera->m_b_orthogonal_)
					{
						float aspectRatio = static_cast<float>(CFG_WIDTH) / static_cast<float>(CFG_HEIGHT);

						camera->m_projection_matrix_ = DirectX::XMMatrixOrthographicLH
								(
								camera->m_fov_ * aspectRatio, camera->m_fov_, CFG_SCREEN_NEAR, CFG_SCREEN_FAR
								);
					}
					else
					{
						camera->m_projection_matrix_ = Managers::D3Device::GetInstance().GetProjectionMatrix();
					}

					const auto invView = camera->m_view_matrix_.Invert();
					const auto invProj = camera->m_projection_matrix_.Invert();

					camera->m_wvp_buffer_.world = camera->m_world_matrix_.Transpose();
					camera->m_wvp_buffer_.view = camera->m_view_matrix_.Transpose();
					camera->m_wvp_buffer_.projection = camera->m_projection_matrix_.Transpose();
					camera->m_wvp_buffer_.invView = invView.Transpose();
					camera->m_wvp_buffer_.invProj = invProj.Transpose();
					camera->m_wvp_buffer_.invVP = XMMatrixTranspose(XMMatrixInverse(nullptr, camera->m_view_matrix_ * camera->m_projection_matrix_));

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

					camera->m_wvp_buffer_.reflectView = XMMatrixLookAtLH
							(
							position,
							position + flipLookAtVector,
							flipUpVector
							);

					camera->m_wvp_buffer_.reflectView = camera->m_wvp_buffer_.reflectView.Transpose();

					// todo: only main camera should do this behaviour.
#if CFG_RAYTRACING
					GetRaytracingPipeline().SetPerspectiveMatrix(m_wvp_buffer_);
#else
					Managers::RenderPipeline::GetInstance().SetPerspectiveMatrix(camera->m_wvp_buffer_);
#endif

					Vector3 velocity = Vector3::Zero;

					if (const auto bound = camera->GetParent().lock())
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
		}
	}

	void CameraManager::Render(const float& dt) {}

	void CameraManager::PostRender(const float& dt) {}

	void CameraManager::PostUpdate(const float& dt) {}

	void    CameraManager::FixedUpdate(const float& dt) {}

	Vector2 CameraManager::GetWorldMousePosition() const
	{
		if (const Strong<Scene>& scene = Managers::SceneManager::GetInstance().GetActiveScene().lock())
		{
			if (const Strong<Objects::Camera>& camera = scene->GetMainCamera().lock())
			{
				const Matrix  pv = Managers::D3Device::GetInstance().GetProjectionMatrix() * camera->m_view_matrix_;
				const Vector2 actual_mouse_position{
					static_cast<float>(Managers::InputManager::GetInstance().GetMouseState().x),
					static_cast<float>(Managers::InputManager::GetInstance().GetMouseState().y)
				};

				const Matrix invProjectionView = XMMatrixInverse(nullptr, pv);

				const float x = (((2.0f * actual_mouse_position.x) / CFG_WIDTH) - 1);
				const float y = -(((2.0f * actual_mouse_position.y) / CFG_HEIGHT) - 1);

				const Vector3 mousePosition = DirectX::XMVectorSet
						(
						 x, y, camera->GetComponent<Components::Transform>().lock()->
						                                             GetWorldPosition().z, 0.0f
						);

				return XMVector3Transform(mousePosition, invProjectionView);
			}
		}

		return {};
	}
}
