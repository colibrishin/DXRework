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
		BoundingFrustum GetFrustum() const { return m_frustum; }

		bool IsInFrustum(const Vector3& position, float radius) const;
		bool IsInFrustum(const Vector3& position, const Vector3& size) const;

	private:
		BoundingFrustum m_frustum;
		BoundingSphere m_sphere;

	};
}
