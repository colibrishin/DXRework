#pragma once
#include "Source/Runtime/Core/Singleton/Public/Singleton.hpp"

namespace Engine::Managers
{
	class PROJECTIONFRUSTUM_API ProjectionFrustum final : public Abstracts::Singleton<ProjectionFrustum>
	{
	public:
		explicit ProjectionFrustum(SINGLETON_LOCK_TOKEN)
			: Singleton() {}

		void Initialize() override;
		void Update(const float& dt) override;
		void PreUpdate(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

		bool CheckRender(const Weak<Abstracts::ObjectBase>& object) const;

		BoundingFrustum GetFrustum() const;

	private:
		friend struct SingletonDeleter;
		~ProjectionFrustum() override = default;

		BoundingFrustum m_frustum;
		BoundingSphere  m_sphere;
	};
} // namespace Engine::Manager
