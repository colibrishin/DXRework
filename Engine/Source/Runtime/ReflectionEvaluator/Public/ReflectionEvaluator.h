#pragma once
#include "Singleton.h"
#include "Texture2D.h"

#include "ReflectionEvaluator.generated.h"

namespace Engine::Managers
{
	ECLASS()
	class ENGINE_REFLECTIONEVALUATOR_API ReflectionEvaluator : public Abstracts::Singleton<ReflectionEvaluator>
	{
		GENERATE_BODY
	public:
		ReflectionEvaluator(SINGLETON_LOCK_TOKEN)
			: Singleton(),
			  m_copy_() {}
		
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void PostUpdate(const float dt) override;
		void Initialize() override;

		void BindReflectionMap(const IGraphicContext* context);
		void UnbindReflectionMap(const IGraphicContext* context);

	private:
		void CheckRender(const eShaderDomain shaderDomain);
		
		friend struct SingletonDeleter;
		~ReflectionEvaluator() override;

		Strong<Resources::Texture2D> m_copy_;
	};
}