#include "ModelRenderer.h"
#include "ModelRenderer.generated.h"



namespace Engine::Components
{
	ModelRenderer::ModelRenderer(const Weak<Engine::Abstracts::ObjectBase>& owner)
		: ShapeRenderComponent(owner) {}

	void ModelRenderer::PreUpdate(const float dt) {}

	void ModelRenderer::Update(const float dt) {}

	void ModelRenderer::FixedUpdate(const float dt) {}

	void ModelRenderer::PostUpdate(const float dt)
	{
		Component::PostUpdate(dt);
	}

	eComponentUpdatePriorities ModelRenderer::GetUpdatePriority() const
	{
		return eComponentUpdatePriority::COM_PRIORITY_RENDER;
	}

	ModelRenderer::ModelRenderer() {}
}
