#include "../Public/ModelRenderer.h"

#include "ModuleManager/Public/ModuleManager.h"
#include "ModelRenderer.generated.h"

namespace Engine::Components
{
	COMP_CLONE_IMPL(ModelRenderer)

	ModelRenderer::ModelRenderer(const Weak<Engine::Abstracts::ObjectBase>& owner)
		: RenderComponent(owner) {}

	void ModelRenderer::PreUpdate(const float dt) {}

	void ModelRenderer::Update(const float dt) {}

	void ModelRenderer::FixedUpdate(const float dt) {}

	void ModelRenderer::PostUpdate(const float dt)
	{
		Component::PostUpdate(dt);
	}

	eComponentUpdatePriority ModelRenderer::GetUpdatePriority() const
	{
		return eComponentUpdatePriority::COM_PRIORITY_RENDER;
	}

	ModelRenderer::ModelRenderer() {}
}
