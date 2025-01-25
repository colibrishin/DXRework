#pragma once
#include "Source/Runtime/CoreSingleton/Public/Singleton.h"
#include "Source/Runtime/Resources/Texture2D/Public/Texture2D.h"

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

		void RenderFinished(const GraphicInterfaceContextPrimitive* context);
		void BindReflectionMap(const GraphicInterfaceContextPrimitive* context);
		void UnbindReflectionMap(const GraphicInterfaceContextPrimitive* context);

	private:
		friend struct SingletonDeleter;
		~ReflectionEvaluator() override = default;

		Strong<Resources::Texture2D> m_copy_;
	};
}