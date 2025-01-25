#include "../Public/ReflectionEvaluator.h"

namespace Engine::Managers
{
	void ReflectionEvaluator::PreUpdate(const float dt) {}

	void ReflectionEvaluator::Update(const float dt) {}

	void ReflectionEvaluator::FixedUpdate(const float dt) {}

	void ReflectionEvaluator::PreRender(const float dt) {}

	void ReflectionEvaluator::Render(const float dt) {}

	void ReflectionEvaluator::PostRender(const float dt) {}

	void ReflectionEvaluator::PostUpdate(const float dt) {}

	void ReflectionEvaluator::Initialize()
	{
		m_copy_ = Resources::Texture2D::Create(
			"Evaluated Reflection", 
			"", 
			GenericTextureDescription {
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
		);

		m_copy_->SetName("ReflectionEvaluator");
		m_copy_->Initialize();
		m_copy_->Load();
	}

	void ReflectionEvaluator::RenderFinished(const GraphicInterfaceContextPrimitive* context)
	{
		GraphicInterfaceAccessor::GetInterface().CopyRenderTarget(context, m_copy_.get());
	}

	void ReflectionEvaluator::BindReflectionMap(const GraphicInterfaceContextPrimitive* context)
	{
		GraphicInterfaceAccessor::GetInterface().Bind(context, m_copy_.get(), BIND_TYPE_SRV, RESERVED_TEX_RENDERED, 0);
	}

	void ReflectionEvaluator::UnbindReflectionMap(const GraphicInterfaceContextPrimitive* context)
	{
		GraphicInterfaceAccessor::GetInterface().TransitBack(context, m_copy_.get(), BIND_TYPE_SRV);
	}
}
