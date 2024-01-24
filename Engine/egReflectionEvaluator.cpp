#include "pch.h"
#include "egReflectionEvaluator.h"

#include "egGlobal.h"

namespace Engine::Manager::Graphics
{
  void ReflectionEvaluator::PreUpdate(const float& dt) { m_copy_.PostRender(0.f); }

  void ReflectionEvaluator::Update(const float& dt) {}

  void ReflectionEvaluator::FixedUpdate(const float& dt) {}

  void ReflectionEvaluator::PreRender(const float& dt) {}

  void ReflectionEvaluator::Render(const float& dt) {}

  void ReflectionEvaluator::PostRender(const float& dt) {}

  void ReflectionEvaluator::PostUpdate(const float& dt) {}

  void ReflectionEvaluator::Initialize()
  {
    m_copy_.Load();
  }

  void ReflectionEvaluator::RenderFinished()
  {
    GetD3Device().CopySwapchain(m_copy_.GetSRV());
    m_copy_.BindAs(D3D11_BIND_SHADER_RESOURCE, RESERVED_RENDERED, 0, SHADER_PIXEL);
    m_copy_.Render(0.f);
  }
}
