#pragma once
#include "egComponent.h"

namespace Engine::Components::Base
{
  class RenderComponent : public Engine::Abstract::Component
  {
  public:
    COMPONENT_T(COM_T_RENDERER)

    explicit RenderComponent(eRenderComponentType type, const WeakObject& owner)
      : Component(COM_T_RENDERER, owner),
        m_mtr_meta_path_(),
        m_type_(type) {}

    void SetMaterial(const WeakMaterial& material) noexcept;

    eRenderComponentType         GetRenderType() const noexcept;
    WeakMaterial                 GetMaterial() const noexcept;
    const std::filesystem::path& GetMaterialMetadataPath() const noexcept;
    eRenderComponentType         GetType() const noexcept;

    void OnDeserialized() override;

  private:
    SERIALIZER_ACCESS
    RenderComponent();

    StrongMaterial m_material_;
    eRenderComponentType  m_type_;
    std::string m_mtr_meta_path_str_;

    // non-serialized
    std::filesystem::path m_mtr_meta_path_;
  };
}

BOOST_CLASS_EXPORT_KEY(Engine::Components::Base::RenderComponent)