#include "pch.h"
#include "egRenderComponent.h"

#include "egMaterial.h"

namespace Engine::Components::Abstract
{
  void RenderComponent::SetMaterial(const WeakMaterial& material) noexcept
  {
    if (const auto mtr = material.lock())
    {
      m_material_      = mtr;
      m_material_name_ = mtr->GetName();
    }
  }

  WeakMaterial RenderComponent::GetMaterial() const noexcept { return m_material_; }

  std::string RenderComponent::GetMaterialName() const noexcept { return m_material_name_; }

  eRenderComponentType RenderComponent::GetType() const noexcept { return m_type_; }

  RenderComponent::RenderComponent()
    : Component(COM_T_RENDERER, {}),
      m_type_(RENDER_COM_T_UNK) {}
}
