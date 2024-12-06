#pragma once
#include "Source/Runtime/Core/Components/RenderComponent/Public/egRenderComponent.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.hpp"
#include "Source/Runtime/Core/StructuredBuffer.h"

namespace Engine::Graphics::SBs
{
	struct InstanceModelSB : public InstanceSB
	{
		InstanceModelSB()
		{
			SetFrame(0.f);
			SetAnimDuration(0);
			SetAnimIndex(0);
			SetNoAnim(false);
			SetWorld(Matrix::Identity);
		}

		void SetFrame(const float frame)
		{
			SetParam(0, frame);
		}

		void SetAnimDuration(const UINT duration)
		{
			SetParam(0, static_cast<int>(duration));
		}

		void SetAnimIndex(const UINT index)
		{
			SetParam(1, static_cast<int>(index));
		}

		void SetNoAnim(const bool no_anim)
		{
			SetParam(2, static_cast<int>(no_anim));
		}

		void SetAtlasX(const UINT x)
		{
			SetParam(3, static_cast<int>(x));
		}

		void SetAtlasY(const UINT y)
		{
			SetParam(4, static_cast<int>(y));
		}

		void SetAtlasW(const UINT w)
		{
			SetParam(5, static_cast<int>(w));
		}

		void SetAtlasH(const UINT h)
		{
			SetParam(6, static_cast<int>(h));
		}

		void SetRepeat(const bool repeat)
		{
			SetParam(7, static_cast<int>(repeat));
		}

		void SetWorld(const Matrix& world)
		{
			SetParam(0, world);
		}
	};
}

namespace Engine::Components
{
	class ModelRenderer final : public RenderComponent
	{
	public:
		RENDER_COM_T(RENDER_COM_T_MODEL)

		ModelRenderer(const Weak<Engine::Abstracts::ObjectBase>& owner);
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

	private:
		SERIALIZE_DECL
		COMP_CLONE_DECL
		friend class Managers::Renderer;

		ModelRenderer();
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Components::ModelRenderer)
