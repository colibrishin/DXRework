#include "pch.h"
#include "egProjectionFrustum.h"
#include "egCamera.h"
#include "egSceneManager.hpp"
#include "egTransform.h"

namespace Engine::Manager
{
	void ProjectionFrustum::Initialize() {}

	void ProjectionFrustum::Update(const float& dt) {}

	void ProjectionFrustum::PreUpdate(const float& dt) {}

	void ProjectionFrustum::PreRender(const float& dt)
	{
		if (const auto scene = GetSceneManager().GetActiveScene().lock())
		{
			const auto camera_layer = scene->GetGameObjects(LAYER_CAMERA);

			if (camera_layer.empty())
			{
				BoundingFrustum::CreateFromMatrix
						(
						 m_frustum,
						 GetD3Device().GetProjectionMatrix()
						);
			}
			else
			{
				const auto camera =
						camera_layer.front().lock()->GetSharedPtr<Objects::Camera>();

				BoundingFrustum::CreateFromMatrix
						(
						 m_frustum,
						 camera->GetProjectionMatrix()
						);
				m_frustum.Transform(m_frustum, camera->GetViewMatrix().Invert());

				BoundingSphere::CreateFromFrustum(m_sphere, m_frustum);
			}
		}
	}

	void ProjectionFrustum::Render(const float& dt) {}

	void ProjectionFrustum::PostRender(const float& dt) {}

	bool ProjectionFrustum::CheckRender(const WeakObjectBase& object) const
	{
		if (const auto tr =
				object.lock()->GetComponent<Components::Transform>().lock())
		{
			BoundingOrientedBox box{
				tr->GetWorldPosition(),
				tr->GetWorldScale() * 0.5f,
				tr->GetWorldRotation()
			};

			try
			{
				const auto check_plane  = m_frustum.Contains(box);
				const auto check_sphere = m_sphere.Contains(box);

				return check_plane != DirectX::DISJOINT ||
				       check_sphere != DirectX::DISJOINT;
			}
			catch (const std::exception& e)
			{
				GetDebugger().Log(e.what());
				return false;
			}
		}

		return false;
	}

	BoundingFrustum ProjectionFrustum::GetFrustum() const
	{
		return m_frustum;
	}

	void ProjectionFrustum::FixedUpdate(const float& dt) {}

	void ProjectionFrustum::PostUpdate(const float& dt) {}
} // namespace Engine::Manager
