#pragma once
#include "Texture2D.h"

#include "ShadowRenderTarget.generated.h"

namespace Engine::Resources
{
	ECLASS(resource, internal)
	class ENGINE_SHADOWRENDERTARGET_API ShadowRenderTarget : public Texture2D
	{
		GENERATE_BODY
	public:
		ShadowRenderTarget()
			: Texture2D
			(
			 "",
			 GenericTextureDescription
			{
				 .Dimension = TEX_TYPE_2D,
				 .Alignment = 0,
				 .Width = CFG_CASCADE_SHADOW_TEX_WIDTH,
				 .Height = CFG_CASCADE_SHADOW_TEX_HEIGHT,
				 .DepthOrArraySize = CFG_CASCADE_SHADOW_COUNT,
				 .Format = TEX_FORMAT_R8G8B8A8_UNORM,
				 .Flags = RESOURCE_FLAG_ALLOW_RENDER_TARGET,
				 .MipsLevel = 1,
				 .Layout = TEX_LAYOUT_UNKNOWN,
				 .SampleDesc = {1, 0},
				 .AsSRV = false,
				 .AsRTV =  true,
			 }
			) { }

		~ShadowRenderTarget() override = default;

		void FixedUpdate(const float dt) override;
		void Initialize() override;
		void PostUpdate(const float dt) override;
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;

		void          OnSerialized() override;
		void          OnDeserialized() override;

		UINT   GetDepth() const override;
		UINT   GetHeight() const override;
		UINT64 GetWidth() const override;

		void Clear(const GraphicInterfaceContextPrimitive* context) const;

	protected:
		void Unload_INTERNAL() override;
	};
}
