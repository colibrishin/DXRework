#include "pch.h"
#include "egReflectionEvaluator.h"

#include "egGlobal.h"

namespace Engine::Manager::Graphics
{
  void ReflectionEvaluator::PreUpdate(const float& dt) {}

  void ReflectionEvaluator::Update(const float& dt) {}

  void ReflectionEvaluator::FixedUpdate(const float& dt) {}

  void ReflectionEvaluator::PreRender(const float& dt) {}

  void ReflectionEvaluator::Render(const float& dt) {}

  void ReflectionEvaluator::PostRender(const float& dt)
  {
    m_copy_.Unbind(COMMAND_LIST_POST_RENDER, BIND_TYPE_SRV);
  }

  void ReflectionEvaluator::PostUpdate(const float& dt) {}

  void ReflectionEvaluator::Initialize()
  {
    m_copy_.SetName("ReflectionEvaluator");
    m_copy_.Initialize();
    m_copy_.Load();
  }

  void ReflectionEvaluator::RenderFinished()
  {
    GetRenderPipeline().CopyBackBuffer(COMMAND_LIST_RENDER, m_copy_.GetRawResoruce());
    m_copy_.Bind(COMMAND_LIST_RENDER, BIND_TYPE_SRV, RESERVED_RENDERED, 0);
  }
}
