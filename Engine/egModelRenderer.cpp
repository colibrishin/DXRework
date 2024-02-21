#include "pch.h"
#include "egModelRenderer.h"

#include "egDebugger.hpp"
#include "egImGuiHeler.hpp"
#include "egMaterial.h"
#include "egShape.h"

SERIALIZER_ACCESS_IMPL
(
 Engine::Components::ModelRenderer,
 _ARTAG(_BSTSUPER(RenderComponent))
)

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
    lldDisabled("Material ID", GetMaterialID());

    if (ImGui::BeginDragDropTarget())
    {
      if (const auto payload = ImGui::AcceptDragDropPayload("RESOURCE"))
      {
        const auto& resource = *static_cast<StrongResource*>(payload->Data);
        if (resource->GetResourceType() == RES_T_MTR)
        {
          SetMaterial(resource->GetSharedPtr<Resources::Material>());
        }
      }

      ImGui::EndDragDropTarget();
    }
  }

  ModelRenderer::ModelRenderer()
    : RenderComponent(RENDER_COM_T_MODEL, {}) {}
}
