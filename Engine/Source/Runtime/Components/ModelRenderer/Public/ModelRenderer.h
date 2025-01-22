#pragma once
#include "Source/Runtime/Components/RenderComponent/Public/egRenderComponent.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.h"

#include "ModuleManager/Public/IModule.h"
#include "ModelRenderer.generated.h"

namespace Engine::Components
{
	ECLASS(serialize)
	class ENGINE_MODELRENDERER_API ModelRenderer final : public RenderComponent
	{
		GENERATE_BODY
	public:
		ModelRenderer(const Weak<Engine::Abstracts::ObjectBase>& owner);
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostUpdate(const float dt) override;

		eComponentUpdatePriority GetUpdatePriority() const override;

	private:
		COMP_CLONE_DECL
		friend class Managers::Renderer;

		ModelRenderer();
	};
}

