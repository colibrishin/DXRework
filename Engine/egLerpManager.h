#pragma once
#include "egManager.hpp"

namespace Engine::Manager::Physics
{
	class LerpManager : public Abstract::Singleton<LerpManager>
	{
	public:
		LerpManager(SINGLETON_LOCK_TOKEN);

		void Initialize() override;
		void Update(const float& dt) override;

		void Reset();
		void PreUpdate(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

		float GetLerpFactor() const;

	private:
		friend struct SingletonDeleter;
		~LerpManager() override = default;

		float m_elapsed_time_;
	};
} // namespace Engine::Manager::Physics


REGISTER_TYPE(Engine::Manager::Application, Engine::Manager::Physics::LerpManager);
