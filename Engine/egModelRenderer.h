#pragma once
#include "egComponent.h"
#include "egRenderComponent.h"

namespace Engine::Components
{
  class ModelRenderer final : public Base::RenderComponent
  {
  public:
    RENDER_COM_T(RENDER_COM_T_MODEL)

    ModelRenderer(const WeakObject& owner);
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void OnImGui() override;

  private:
    SERIALIZE_DECL
    COMP_CLONE_DECL
    friend class Manager::Graphics::Renderer;

    ModelRenderer();

  };
}

BOOST_CLASS_EXPORT_KEY(Engine::Components::ModelRenderer)