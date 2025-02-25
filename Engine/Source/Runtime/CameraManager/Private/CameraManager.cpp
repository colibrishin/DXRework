#include "CameraManager.h"
#include "CameraManager.generated.h"

#include "Rigidbody.h"
#include "Transform.h"
#include "Camera.h"
#include "SceneManager.h"

#include "GraphicInterface.h"

namespace Engine::Managers
{
	void CameraManager::Initialize() {}

	void CameraManager::PreUpdate(const float dt) {}

	void CameraManager::Update(const float dt) {}

	void CameraManager::PreRender(const float dt) {}

	void CameraManager::Render(const float dt) {}

	void CameraManager::PostRender(const float dt) {}

	void CameraManager::PostUpdate(const float dt)
	{
		if (const Strong<Scene>& scene = SceneManager::GetInstance().GetActiveScene().lock()) 
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
							position + forward,
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
						camera->m_projection_matrix_ = GraphicInterfaceAccessor::GetInterface().GetProjectionMatrix();
					}

					const auto invView = camera->m_view_matrix_.Invert();
					const auto invProj = camera->m_projection_matrix_.Invert();

					camera->m_perspective_cb_.world = camera->m_world_matrix_.Transpose();
					camera->m_perspective_cb_.view = camera->m_view_matrix_.Transpose();
					camera->m_perspective_cb_.projection = camera->m_projection_matrix_.Transpose();
					camera->m_perspective_cb_.invView = invView.Transpose();
					camera->m_perspective_cb_.invProj = invProj.Transpose();
					camera->m_perspective_cb_.invVP = XMMatrixTranspose(XMMatrixInverse(nullptr, camera->m_view_matrix_ * camera->m_projection_matrix_));

					// do the same with mirror rotation
					// flip backward, and roll forward
					Matrix flipRotation = Matrix::Transform
					(
						rotationMatrix, MathExtension::ToQuaternion(DirectX::XMConvertToRadians(180.f), 0, DirectX::XMConvertToRadians(180.f))
					);

					Vector3 flipLookAtVector = XMVector3TransformNormal(forward, flipRotation);
					Vector3 flipUpVector     = XMVector3TransformNormal(up, flipRotation);

					camera->m_perspective_cb_.reflectView = XMMatrixLookAtLH
							(
							position,
							position + flipLookAtVector,
							flipUpVector
							);

					camera->m_perspective_cb_.reflectView = camera->m_perspective_cb_.reflectView.Transpose();
				}
			}
		}
	}

	void CameraManager::FixedUpdate(const float dt) {}
}
