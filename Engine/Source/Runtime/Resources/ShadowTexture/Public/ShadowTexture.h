#pragma once
#include "Source/Runtime/Resources/Texture2D/Public/Texture2D.h"

namespace Engine::Resources
{
	class SHADOWTEXTURE_API ShadowTexture : public Texture2D
	{
	public:
		RESOURCE_T(RES_T_SHADOW_TEX)

		ShadowTexture()
			: Texture2D
			(
			 "",
			 GenericTextureDescription
			{
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
				 .AsRTV = false,
				 .AsDSV = true,
				 .AsUAV = false,
				 .Srv = {
					.Format = TEX_FORMAT_R32_FLOAT,
					.ViewDimension = SRV_DIMENSION_TEXTURE2DARRAY,
					.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
					.Texture2DArray = {
						.MostDetailedMip = 0,
						.MipLevels = 1,
						.FirstArraySlice = 0,
						.ArraySize = CFG_CASCADE_SHADOW_COUNT,
						.PlaneSlice = 0,
						.ResourceMinLODClamp = 0.f
					},
				 },
				.Rtv = {},
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

		void FixedUpdate(const float& dt) override;
		void Initialize() override;
		void PostUpdate(const float& dt) override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;

		void          OnSerialized() override;
		void          OnDeserialized() override;
		eResourceType GetResourceType() const override;

		UINT   GetDepth() const override;
		UINT   GetHeight() const override;
		UINT64 GetWidth() const override;

		void Clear(ID3D12GraphicsCommandList1* cmd) const;

	protected:
		void Unload_INTERNAL() override;
	};
}
