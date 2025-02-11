#pragma once
#include "Source/Runtime/Components/RenderComponent/Public/egRenderComponent.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.h"

#include "ModuleManager/Public/IModule.h"
#include "ModelRenderer.generated.h"

namespace Engine::Components
{
	ECLASS(serialize, component)
	class ENGINE_MODELRENDERER_API ModelRenderer final : public RenderComponent
	{
		GENERATE_BODY
	public:
		ModelRenderer(const Weak<Engine::Abstracts::ObjectBase>& owner);
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostUpdate(const float dt) override;

		eComponentUpdatePriorities GetUpdatePriority() const override;

	private:
		friend class Managers::Renderer;

		ModelRenderer();
	};
}

