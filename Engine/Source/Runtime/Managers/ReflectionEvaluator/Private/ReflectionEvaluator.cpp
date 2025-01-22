#include "../Public/ReflectionEvaluator.h"

namespace Engine::Managers
{
	void ReflectionEvaluator::PreUpdate(const float& dt) {}

	void ReflectionEvaluator::Update(const float& dt) {}

	void ReflectionEvaluator::FixedUpdate(const float& dt) {}

	void ReflectionEvaluator::PreRender(const float& dt) {}

	void ReflectionEvaluator::Render(const float& dt) {}

	void ReflectionEvaluator::PostRender(const float& dt) {}

	void ReflectionEvaluator::PostUpdate(const float& dt) {}

	void ReflectionEvaluator::Initialize()
	{
		m_copy_.SetName("ReflectionEvaluator");
		m_copy_.Initialize();
		m_copy_.Load();
	}

	void ReflectionEvaluator::RenderFinished(const GraphicInterfaceContextPrimitive* context)
	{
		g_graphic_interface.GetInterface().CopyRenderTarget(context, &m_copy_);
	}

	void ReflectionEvaluator::BindReflectionMap(const GraphicInterfaceContextPrimitive* context)
	{
		g_graphic_interface.GetInterface().Bind(context, &m_copy_, BIND_TYPE_SRV, RESERVED_TEX_RENDERED, 0);
	}

	void ReflectionEvaluator::UnbindReflectionMap(const GraphicInterfaceContextPrimitive* context)
	{
		g_graphic_interface.GetInterface().Unbind(context, &m_copy_, BIND_TYPE_SRV);
	}
}
