#pragma once
#include "egCommon.hpp"
#include "egDXCommon.h"

#include "egSceneManager.hpp"
#include "egCamera.hpp"
#include "egD3Device.hpp"
#include "egLayer.hpp"
#include "egManager.hpp"
#include "egTransform.hpp"

namespace Engine::Manager
{
	class ProjectionFrustum final : public Abstract::Singleton<ProjectionFrustum>
	{
	public:
		explicit ProjectionFrustum(SINGLETON_LOCK_TOKEN) : Singleton() {}
		~ProjectionFrustum() override = default;

		void Initialize() override;
		void Update(const float& dt) override;
		void PreUpdate(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		bool CheckRender(const WeakObject& object) const;

		bool IsInFrustum(const Vector3& position, float radius) const;
		bool IsInFrustum(const Vector3& position, const Vector3& size) const;

	private:
		BoundingFrustum m_frustum;
		BoundingSphere m_sphere;

	};

	inline void ProjectionFrustum::Initialize()
	{
	}

	inline void ProjectionFrustum::Update(const float& dt)
	{
	}

	inline void ProjectionFrustum::PreUpdate(const float& dt)
	{
	}

	inline void ProjectionFrustum::PreRender(const float& dt)
	{
		if (const auto scene = GetSceneManager().GetActiveScene().lock())
		{
			const auto camera_layer = scene->GetGameObjects(LAYER_CAMERA);

			if (camera_layer.empty())
			{
				BoundingFrustum::CreateFromMatrix(m_frustum, GetD3Device().GetProjectionMatrix());
			}
			else
			{
				const auto camera = camera_layer.front().lock()->GetSharedPtr<Objects::Camera>();

				BoundingFrustum::CreateFromMatrix(m_frustum, GetD3Device().GetProjectionMatrix());

				m_frustum.Origin = camera->GetComponent<Component::Transform>().lock()->GetPosition();
				m_frustum.Orientation = Quaternion::CreateFromYawPitchRoll(camera->GetLookAt());

				BoundingSphere::CreateFromFrustum(m_sphere, m_frustum);
			}
		}
	}

	inline void ProjectionFrustum::Render(const float& dt)
	{
	}

	inline bool ProjectionFrustum::CheckRender(const WeakObject& object) const
	{
		if(const auto tr = object.lock()->GetComponent<Component::Transform>().lock())
		{
			BoundingOrientedBox box
			{
				tr->GetPosition(),
				tr->GetScale() * 0.5f,
				tr->GetRotation()
			};

			const auto check_plane = m_frustum.Intersects(box);
			const auto check_sphere = m_sphere.Intersects(box);

			return check_plane || check_sphere;
		}

		return false;
	}

	inline void ProjectionFrustum::FixedUpdate(const float& dt)
	{
	}
}
