#pragma once
#include "Singleton.h"

#include "Graviton.generated.h"

namespace Engine::Managers
{
    ECLASS()
	class ENGINE_PHYSICSMANAGER_API Graviton : public Abstracts::Singleton<Graviton>
	{
        GENERATE_BODY

	public:
		Graviton(SINGLETON_LOCK_TOKEN) {}

		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PostUpdate(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void Initialize() override;

	private:
		friend struct SingletonDeleter;
        friend struct ConstructorAccess;

		~Graviton() override = default;
	};
}