#include "../Public/ModelRenderer.h"

SERIALIZE_IMPL
(
 Engine::Components::ModelRenderer,
 _ARTAG(_BSTSUPER(RenderComponent))
)

namespace Engine::Components
{
	COMP_CLONE_IMPL(ModelRenderer)

	ModelRenderer::ModelRenderer(const Weak<Engine::Abstracts::ObjectBase>& owner)
		: RenderComponent(RENDER_COM_T_MODEL, owner) {}

	void ModelRenderer::PreUpdate(const float& dt) {}

	void ModelRenderer::Update(const float& dt) {}

	void ModelRenderer::FixedUpdate(const float& dt) {}

	void ModelRenderer::PostUpdate(const float& dt)
	{
		Component::PostUpdate(dt);
	}

	ModelRenderer::ModelRenderer()
		: RenderComponent(RENDER_COM_T_MODEL, {}) {}
}
