#include "pch.h"
#include "egModelRenderer.h"

#include "egDebugger.hpp"
#include "egImGuiHeler.hpp"
#include "egMaterial.h"
#include "egShape.h"

namespace Engine::Components
{
  ModelRenderer::ModelRenderer(const WeakObject& owner)
    : Component(COM_T_MODEL_RENDERER, owner) {}

  void ModelRenderer::PreUpdate(const float& dt) {}

  void ModelRenderer::Update(const float& dt) {}

  void ModelRenderer::FixedUpdate(const float& dt) {}

  void ModelRenderer::PostUpdate(const float& dt) { Component::PostUpdate(dt); }

  void ModelRenderer::OnImGui()
  {
    Component::OnImGui();
    TextDisabled("Material Name", m_material_name_);
  }

  void ModelRenderer::SetMaterial(const WeakMaterial& material)
  {
    if (const auto m = material.lock())
    {
      m_material_      = m;
      m_material_name_ = m->GetName();
    }
  }

  WeakMaterial ModelRenderer::GetMaterial() const
  {
    if (m_material_) { return m_material_; }

    return {};
  }

  ModelRenderer::ModelRenderer()
    : Component(COM_T_MODEL_RENDERER, {}) {}
}
