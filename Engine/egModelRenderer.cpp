#include "pch.h"
#include "egModelRenderer.h"

#include "egDebugger.hpp"
#include "egImGuiHeler.hpp"
#include "egMaterial.h"
#include "egShape.h"

namespace Engine::Components
{
  ModelRenderer::ModelRenderer(const WeakObject& owner)
    : RenderComponent(RENDER_COM_T_MODEL, owner) {}

  void ModelRenderer::PreUpdate(const float& dt) {}

  void ModelRenderer::Update(const float& dt) {}

  void ModelRenderer::FixedUpdate(const float& dt) {}

  void ModelRenderer::PostUpdate(const float& dt) { Component::PostUpdate(dt); }

  void ModelRenderer::OnImGui()
  {
    Component::OnImGui();
    TextDisabled("Material Name", GetMaterialName());
  }

  ModelRenderer::ModelRenderer()
    : RenderComponent(RENDER_COM_T_MODEL, {}) {}
}
