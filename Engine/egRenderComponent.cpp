#include "pch.h"
#include "egRenderComponent.h"

#include "egImGuiHeler.hpp"
#include "egMaterial.h"

SERIALIZE_IMPL
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

  void RenderComponent::OnSerialized()
  {
    if (m_material_)
    {
      Serializer::Serialize(m_material_->GetName(), m_material_);
      m_mtr_meta_path_str_ = m_material_->GetMetadataPath().string();
    }
  }

  void RenderComponent::OnDeserialized()
  {
    Component::OnDeserialized();
    if (const auto res_check = Resources::Material::GetByMetadataPath(m_mtr_meta_path_).lock(); 
        res_check && !res_check->GetMetadataPath().empty())
    {
      m_material_ = res_check;
    }
    else
    {
      m_material_ = GetResourceManager().GetResourceByMetadataPath<Resources::Material>(m_mtr_meta_path_str_).lock();
    }
  }

  void RenderComponent::OnImGui()
  {
    Component::OnImGui();
    std::string format;

    if (m_material_)
    {
      format = std::format("{} ({})", m_material_->GetName(), m_mtr_meta_path_str_);
    }

    TextDisabled("Material", format.c_str());
    if (ImGui::BeginDragDropTarget())
    {
      if (const auto payload = ImGui::AcceptDragDropPayload("RESOURCE"))
      {
        const StrongResource res = *static_cast<StrongResource*>(payload->Data);
        if (const auto mtr = boost::dynamic_pointer_cast<Resources::Material>(res))
        {
          mtr->Load();
          m_material_          = mtr;
          m_mtr_meta_path_str_ = mtr->GetMetadataPath().string();
        }
      }
    }
  }

  RenderComponent::RenderComponent()
    : Component(COM_T_RENDERER, {}),
      m_type_(RENDER_COM_T_UNK) {}
}
