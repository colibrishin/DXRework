#pragma once
#include "Source/Runtime/Components/RenderComponent/Public/egRenderComponent.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.hpp"
#include "Source/Runtime/Core/StructuredBuffer.h"

namespace Engine::Graphics::SBs
{
	struct ENGINE_MODELRENDERER_API InstanceModelSB : public InstanceSB
	{
		InstanceModelSB();

		void SetFrame(const float frame);
		void SetAnimDuration(const UINT duration);
		void SetAnimIndex(const UINT index);
		void SetNoAnim(const bool no_anim);
		void SetAtlasX(const UINT x);
		void SetAtlasY(const UINT y);
		void SetAtlasW(const UINT w);
		void SetAtlasH(const UINT h);
		void SetRepeat(const bool repeat);
		void SetWorld(const Matrix& world);
	};
}

POLYMORPHIC_TYPE_MAP(Engine::Components::ModelRenderer, Engine::Components::RenderComponent)

namespace Engine::Components
{
	class ENGINE_MODELRENDERER_API ModelRenderer final : public RenderComponent
	{
	public:
		INLINE_COMPILE_TIME_TYPENAME(ModelRenderer)

		ModelRenderer(const Weak<Engine::Abstracts::ObjectBase>& owner);
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostUpdate(const float dt) override;

	private:
		COMP_CLONE_DECL
		friend class Managers::Renderer;

		ModelRenderer();
	};
}

