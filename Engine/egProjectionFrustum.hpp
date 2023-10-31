#pragma once
#include "egD3Device.hpp"
#include "egLayer.hpp"
#include "egManager.hpp"
#include "egManagerHelper.hpp"
#include "egTransform.hpp"

namespace Engine::Manager
{
	class ProjectionFrustum final : public Abstract::Manager<ProjectionFrustum>
	{
	public:
		explicit ProjectionFrustum(SINGLETON_LOCK_TOKEN) : Manager() {}
		~ProjectionFrustum() override = default;

		void Initialize() override;
		void Update() override;
		void PreUpdate() override;
		void PreRender() override;
		void Render() override;
		void FixedUpdate() override;

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

	inline void ProjectionFrustum::Update()
	{
	}

	inline void ProjectionFrustum::PreUpdate()
	{
	}

	inline void ProjectionFrustum::PreRender()
	{
		if (const auto scene = GetSceneManager()->GetActiveScene().lock())
		{
			const auto camera_layer = scene->GetGameObjects(LAYER_CAMERA);

			if (camera_layer.empty())
			{
				BoundingFrustum::CreateFromMatrix(m_frustum, Graphic::D3Device::GetProjectionMatrix());
			}
			else
			{
				const auto camera = camera_layer.front().lock()->GetSharedPtr<Objects::Camera>();

				BoundingFrustum::CreateFromMatrix(m_frustum, camera->GetViewMatrix() * Graphic::D3Device::GetProjectionMatrix());

				m_frustum.Origin = camera->GetComponent<Component::Transform>().lock()->GetPosition() + camera->GetLookAt();

				BoundingSphere::CreateFromFrustum(m_sphere, m_frustum);
			}
		}
	}

	inline void ProjectionFrustum::Render()
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

			const auto check_plane = m_frustum.Contains(box);
			const auto check_sphere = m_sphere.Contains(box);

			if (check_plane != ContainmentType::DISJOINT || 
				check_sphere != ContainmentType::DISJOINT)
			{
				return true;
			}

			return false;
		}

		return false;
	}

	inline void ProjectionFrustum::FixedUpdate()
	{
	}
}
