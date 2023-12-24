#include "pch.h"
#include "egReflectionEvaluator.h"

namespace Engine::Manager::Graphics
{
    void ReflectionEvaluator::PreUpdate(const float& dt)
    {
        GetRenderPipeline().UnbindResource(SR_RENDERED);
    }

    void ReflectionEvaluator::Update(const float& dt) {}

    void ReflectionEvaluator::FixedUpdate(const float& dt) {}

    void ReflectionEvaluator::PreRender(const float& dt) {}

    void ReflectionEvaluator::Render(const float& dt) {}

    void ReflectionEvaluator::PostRender(const float& dt)
    {
        GetRenderPipeline().BindResource(SR_RENDERED, SHADER_PIXEL, m_rendered_buffer_.srv.Get());
    }

    void ReflectionEvaluator::Initialize()
    {
    }

    void ReflectionEvaluator::ReceiveRenderFinished()
    {
        GetD3Device().GetSwapchainCopy(m_rendered_buffer_);
    }
}
