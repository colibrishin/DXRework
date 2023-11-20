#pragma once

namespace Engine::Manager::Physics
{
	class TransformLerpManager : public Abstract::Singleton<TransformLerpManager>
	{
	public:
		TransformLerpManager(SINGLETON_LOCK_TOKEN) : Singleton(), m_elapsedTime_(0.f) {}
		~TransformLerpManager() override = default;

		void Initialize() override;
		void Update(const float& dt) override;

		void Reset();
		void PreUpdate(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		float GetLerpFactor() const;

	private:
		float m_elapsedTime_;

	};
}
