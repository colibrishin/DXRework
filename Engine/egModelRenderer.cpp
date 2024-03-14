#include "pch.h"
#include "egModelRenderer.h"

#include "egDebugger.hpp"
#include "egImGuiHeler.hpp"
#include "egMaterial.h"
#include "egShape.h"

SERIALIZE_IMPL
(
 Engine::Components::ModelRenderer,
 _ARTAG(_BSTSUPER(RenderComponent))
)

namespace Engine::Components
{
  COMP_CLONE_IMPL(ModelRenderer)

  ModelRenderer::ModelRenderer(const WeakObject& owner)
    : RenderComponent(RENDER_COM_T_MODEL, owner) {}

  void ModelRenderer::PreUpdate(const float& dt) {}

  void ModelRenderer::Update(const float& dt) {}

  void ModelRenderer::FixedUpdate(const float& dt) {}

  void ModelRenderer::PostUpdate(const float& dt) { Component::PostUpdate(dt); }

  void ModelRenderer::OnImGui()
  {
    RenderComponent::OnImGui();
  }

  ModelRenderer::ModelRenderer()
    : RenderComponent(RENDER_COM_T_MODEL, {}) {}
}
