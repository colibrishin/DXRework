#include "CameraManager.h"

#include "Rigidbody.h"
#include "Transform.h"
#include "Camera.h"
#include "SceneManager.h"

#include "IGraphicAPI_Extensions.h"

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
					Vector3    up       = camera->GetFixedUp() ? Vector3::Up : transform->Up();
					Vector3    forward  = transform->Forward();

					Matrix rotationMatrix = Matrix::CreateFromQuaternion(rotation);

					camera->SetWorldMatrix(
							Matrix::CreateWorld(Vector3::Zero, g_forward, Vector3::Up) *
							rotationMatrix *
							Matrix::CreateTranslation(position));

					// Finally create the view matrix from the three updated vectors.
					camera->SetViewMatrix(XMMatrixLookAtLH
							(
							position,
							position + forward,
							up
							));

					if (camera->GetOrthogonal())
					{
						float aspectRatio = static_cast<float>(CFG_WIDTH) / static_cast<float>(CFG_HEIGHT);

						camera->SetProjectionMatrix(DirectX::XMMatrixOrthographicLH
								(
								camera->GetFOV() * aspectRatio, camera->GetFOV(), CFG_SCREEN_NEAR, CFG_SCREEN_FAR
								));
					}
					else
					{
						camera->SetProjectionMatrix(g_graphic_accessor.GetInterface().GetProjectionMatrix());
					}

					const auto invView = camera->GetViewMatrix().Invert();
					const auto invProj = camera->GetProjectionMatrix().Invert();

					Graphics::CBs::PerspectiveCB cb = camera->GetPerspectiveCB();
					cb.world = camera->GetWorldMatrix().Transpose();
					cb.view = camera->GetViewMatrix().Transpose();
					cb.projection = camera->GetProjectionMatrix().Transpose();
					cb.invView = invView.Transpose();
					cb.invProj = invProj.Transpose();
					cb.invVP = XMMatrixTranspose(XMMatrixInverse(nullptr, camera->GetViewMatrix() * camera->GetProjectionMatrix()));

					// do the same with mirror rotation
					// flip backward, and roll forward
					Matrix flipRotation = Matrix::Transform
					(
						rotationMatrix, MathExtension::ToQuaternion(DirectX::XMConvertToRadians(180.f), 0, DirectX::XMConvertToRadians(180.f))
					);

					Vector3 flipLookAtVector = XMVector3TransformNormal(forward, flipRotation);
					Vector3 flipUpVector     = XMVector3TransformNormal(up, flipRotation);

					cb.reflectView = XMMatrixLookAtLH
							(
							position,
							position + flipLookAtVector,
							flipUpVector
							);

					cb.reflectView = cb.reflectView.Transpose();
					camera->SetPerspectiveCB(cb);
				}
			}
		}
	}

	void CameraManager::FixedUpdate(const float dt) {}
}
