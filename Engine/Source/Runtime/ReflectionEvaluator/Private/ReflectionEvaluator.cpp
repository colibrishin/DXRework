#include "ReflectionEvaluator.h"
#include "ReflectionEvaluator.generated.h"

#include "Renderer.h"

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
			});

		Renderer::GetInstance().onRenderDone.Listen(
			GetSharedPtr<ReflectionEvaluator>(),
			&ReflectionEvaluator::CheckRender);
	}

	void ReflectionEvaluator::BindReflectionMap(const IGraphicContext* context)
	{
		g_graphic_accessor.GetInterface().Bind(context, m_copy_.get(), BIND_TYPE_SRV, RESERVED_TEX_RENDERED, 0);
	}

	void ReflectionEvaluator::UnbindReflectionMap(const IGraphicContext* context)
	{
		g_graphic_accessor.GetInterface().TransitBack(context, m_copy_.get(), BIND_TYPE_SRV);
	}

	void ReflectionEvaluator::CheckRender(const eShaderDomain shaderDomain)
	{
		if (shaderDomain == SHADER_DOMAIN_OPAQUE)
		{
			IGraphicAPI& gi = g_graphic_accessor.GetInterface();
			const auto& context = gi.GetNewContext(0, false, L"Opaque render target copy");
			const auto& primitive = context.GetPointers();
			primitive.commandList->SoftReset();
			gi.CopyRenderTarget(&primitive, m_copy_.get());
			primitive.commandList->FlagReady();
		}
	}

	ReflectionEvaluator::~ReflectionEvaluator()
	{
        Renderer::GetInstance().onRenderDone.Remove( GetWeakPtr<ReflectionEvaluator>(),
                                                     &ReflectionEvaluator::CheckRender );
	}
}
