#pragma once
#include <egTexture2D.h>

namespace Client::Resource
{
	class IntensityTexture final : public Resources::Texture2D
	{
	public:
		IntensityTexture()
			: Texture2D
			(
			 "", {
				 .Alignment = 0,
				 .Width = g_max_shadow_map_size,
				 .Height = g_max_shadow_map_size,
				 .DepthOrArraySize = 1,
				 .Format = DXGI_FORMAT_R32G32B32A32_UINT,
				 .Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
				 .MipsLevel = 1,
				 .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
				 .SampleDesc = {1, 0},
			 }
			) { }
	};
}
