#pragma once
#include "Texture2D.h"

#include "ShadowTexture.generated.h"

namespace Engine::Resources
{
	ECLASS(resource, internal)
	class ENGINE_SHADOWTEXTURE_API ShadowTexture : public Texture2D
	{
		GENERATE_BODY
	public:
		ShadowTexture()
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
				 .Format = TEX_FORMAT_R32_TYPELESS,
				 .Flags = RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
				 .MipsLevel = 1,
				 .Layout = TEX_LAYOUT_UNKNOWN,
				 .SampleDesc = {1, 0},
				 .AsSRV = true,
				 .AsDSV = true,
				 .Srv = {
					.Format = TEX_FORMAT_R32_FLOAT,
					.ViewDimension = SRV_DIMENSION_TEXTURE2DARRAY,
					.Shader4ComponentMapping = d3d12_shader4_component_mapping,
					.Texture2DArray = {
						.MostDetailedMip = 0,
						.MipLevels = 1,
						.FirstArraySlice = 0,
						.ArraySize = CFG_CASCADE_SHADOW_COUNT,
						.PlaneSlice = 0,
						.ResourceMinLODClamp = 0.f
					},
				 },
				.Dsv = {
					.Format = TEX_FORMAT_D32_FLOAT,
					.ViewDimension = DSV_DIMENSION_TEXTURE2DARRAY,
					.Flags = DSV_FLAG_NONE,
					.Texture2DArray = {
						.MipSlice = 0,
						.FirstArraySlice = 0,
						.ArraySize = CFG_CASCADE_SHADOW_COUNT
					}
				}
			 }
			) { }

		~ShadowTexture() override = default;

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

		void Clear(const IGraphicContext* context) const;

	protected:
		void Unload_INTERNAL() override;
	};
}
