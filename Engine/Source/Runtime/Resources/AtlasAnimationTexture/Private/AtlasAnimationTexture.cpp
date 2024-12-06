#include "../Public/AtlasAnimationTexture.h"

#if defined(USE_DX12)
#include "Source/Runtime/CommandPair/Public/CommandPair.h"
#endif

#include <DirectXTex.h>


SERIALIZE_IMPL
(
 Engine::Resources::AtlasAnimationTexture,
 _ARTAG(_BSTSUPER(Engine::Resources::Texture3D))
 _ARTAG(m_atlases_)
)

namespace Engine::Resources
{
	AtlasAnimationTexture::AtlasAnimationTexture(
		const boost::filesystem::path& path, const std::vector<Strong<Texture2D>>& atlases
	)
		: Texture3D(path, {}),
		  m_atlases_(atlases) {}

	void AtlasAnimationTexture::PreUpdate(const float& dt) {}

	void AtlasAnimationTexture::Update(const float& dt) {}

	void AtlasAnimationTexture::FixedUpdate(const float& dt) {}

	void AtlasAnimationTexture::PostUpdate(const float& dt) {}

	void AtlasAnimationTexture::OnSerialized()
	{
		Texture3D::OnSerialized();

		// Backup the atlases. This could be removed.
		for (const auto& atlas : m_atlases_)
		{
			Serializer::Serialize(atlas->GetName(), atlas);
		}
	}

	void AtlasAnimationTexture::OnDeserialized()
	{
		Texture3D::OnDeserialized();
	}

	eResourceType AtlasAnimationTexture::GetResourceType() const
	{
		return RES_T_ATLAS_TEX;
	}

	bool AtlasAnimationTexture::DoesWantMapByResource() const
	{
		return true;
	}

	void AtlasAnimationTexture::loadDerived(ComPtr<ID3D12Resource>& res)
	{
		if (m_atlases_.size() > std::numeric_limits<UINT16>::max())
		{
			OutputDebugStringW(L"Warning: atlas textures are given more than limit 65535\n");
		}

		const UINT16 num_atlases = static_cast<UINT16>(m_atlases_.size());

		// Build the atlas from textures if no path is provided. (assuming that this has not been serialized before)
		if (GetPath().empty())
		{
			UINT64 width  = 0;
			UINT   height = 0;

			// Find the largest atlas to set the 3D texture dimensions.
			for (UINT16 i = 0; i < num_atlases; ++i)
			{
				m_atlases_[i]->Load();
				width  = std::max(width, m_atlases_[i]->GetWidth());
				height = std::max(height, m_atlases_[i]->GetHeight());
			}

			LazyDescription
					(
					 {
						 .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D,
						 .Alignment = 0,
						 .Width = width,
						 .Height = height,
						 .DepthOrArraySize = num_atlases,
						 .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
						 .Flags = D3D12_RESOURCE_FLAG_NONE,
						 .MipsLevel = 1,
						 .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
						 .SampleDesc = {1, 0}
					 }
					);
		}

		Texture3D::loadDerived(res);
	}

	bool AtlasAnimationTexture::map(const Weak<CommandPair>& w_cmd, ID3D12Resource* texture_resource)
	{
		// See DepthOrArraySize for type.
		const UINT16 num_atlases = static_cast<UINT16>(m_atlases_.size());

		if (GetPath().empty())
		{
			const Strong<CommandPair>& cmd = w_cmd.lock();

			// Merge all the atlases into the 3D texture.
			// Keep the atlases dimensions, progress from left to right, top to bottom
			// for matching with xml data.
			for (UINT16 i = 0; i < num_atlases; ++i)
			{
				ComPtr<ID3D12Resource> anim = m_atlases_[i]->GetRawResoruce();
				D3D12_BOX box = {0, 0, 0, static_cast<UINT>(m_atlases_[i]->GetWidth()), m_atlases_[i]->GetHeight(), 1};

				D3D12_TEXTURE_COPY_LOCATION dst
				{
					.pResource = texture_resource,
					.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
					.SubresourceIndex = 0
				};

				D3D12_TEXTURE_COPY_LOCATION src
				{
					.pResource = anim.Get(),
					.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
					.SubresourceIndex = 0
				};

				cmd->GetList()->CopyTextureRegion
						(
						 &dst,
						 0,
						 0,
						 i,
						 &src,
						 &box
						);
			}

			return true;
		}

		return false;
		
	}
}
