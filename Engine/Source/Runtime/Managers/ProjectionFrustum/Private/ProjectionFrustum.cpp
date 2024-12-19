#include "../Public/ProjectionFrustum.h"
#include "Source/Runtime/Core/SceneManager/Public/SceneManager.hpp"
#include "Source/Runtime/Core/Objects/Camera/Public/Camera.h"
#include "Source/Runtime/Core/Scene/Public/Scene.hpp"
#include "Source/Runtime/Core/Components/Transform/Public/Transform.h"

#if WITH_DEBUG
#include "Source/Runtime/Managers/Debugger/Public/Debugger.hpp"
#endif

namespace Engine::Managers
{
	void ProjectionFrustum::Initialize() {}

	void ProjectionFrustum::Update(const float& dt) {}

	void ProjectionFrustum::PreUpdate(const float& dt) {}

	void ProjectionFrustum::PreRender(const float& dt)
	{
		if (const auto scene = Managers::SceneManager::GetInstance().GetActiveScene().lock())
		{
			const auto camera_layer = scene->GetGameObjects(RESERVED_LAYER_CAMERA);

			if (camera_layer.empty())
			{
				BoundingFrustum::CreateFromMatrix
						(
						 m_frustum,
						 g_graphic_interface.GetInterface().GetProjectionMatrix()
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

	bool ProjectionFrustum::CheckRender(const Weak<Abstracts::ObjectBase>& object) const
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
#if WITH_DEBUG
				Managers::Debugger::GetInstance().Log(e.what());
#endif
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
