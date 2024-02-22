#include "pch.h"
#include "egRenderComponent.h"

#include "egMaterial.h"

SERIALIZER_ACCESS_IMPL
(
 Engine::Components::Base::RenderComponent,
 _ARTAG(_BSTSUPER(Engine::Abstract::Component))
 _ARTAG(m_mtr_meta_path_str_)
 _ARTAG(m_type_)
)

namespace Engine::Components::Base
{
  void RenderComponent::SetMaterial(const WeakMaterial& material) noexcept
  {
    if (const auto mtr = material.lock())
    {
      m_material_    = mtr;
      m_mtr_meta_path_ = mtr->GetMetadataPath();
    }
  }

  eRenderComponentType RenderComponent::GetRenderType() const noexcept { return m_type_; }

  WeakMaterial RenderComponent::GetMaterial() const noexcept { return m_material_; }

  const std::filesystem::path& RenderComponent::GetMaterialMetadataPath() const noexcept { return m_mtr_meta_path_str_; }

  eRenderComponentType RenderComponent::GetType() const noexcept { return m_type_; }

  void RenderComponent::OnDeserialized()
  {
    Component::OnDeserialized();
    m_material_ = Resources::Material::GetByMetadataPath(m_mtr_meta_path_).lock();
  }

  RenderComponent::RenderComponent()
    : Component(COM_T_RENDERER, {}),
      m_type_(RENDER_COM_T_UNK) {}
}
