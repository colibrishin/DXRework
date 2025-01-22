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
			  m_copy_
			  (
			   "", {
				   .Alignment = 0,
				   .Width = CFG_WIDTH,
				   .Height = CFG_HEIGHT,
				   .DepthOrArraySize = 1,
				   .Format = TEX_FORMAT_R8G8B8A8_UNORM,
				   .Flags = RESOURCE_FLAG_NONE,
				   .MipsLevel = 1,
				   .Layout = TEX_LAYOUT_UNKNOWN,
				   .SampleDesc = {.Count = 1, .Quality = 0}
			   }
			  ) {}

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

		Resources::Texture2D m_copy_;
	};
}