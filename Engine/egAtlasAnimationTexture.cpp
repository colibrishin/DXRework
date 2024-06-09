#include "pch.h"
#include "egAtlasAnimationTexture.h"

#include "egTexture2D.h"

SERIALIZE_IMPL
(
 Engine::Resources::AtlasAnimationTexture,
 _ARTAG(_BSTSUPER(Engine::Resources::Texture3D))
 _ARTAG(m_atlases_)
)

namespace Engine::Resources
{
  AtlasAnimationTexture::AtlasAnimationTexture(const std::filesystem::path& path, const std::vector<StrongTexture2D>& atlases)
    : Texture3D(path, {}), m_atlases_(atlases) {}

  void AtlasAnimationTexture::PreUpdate(const float& dt) {}

  void AtlasAnimationTexture::Update(const float& dt) {}

  void AtlasAnimationTexture::FixedUpdate(const float& dt) {}

  void AtlasAnimationTexture::PreRender(const float& dt) { Texture3D::PreRender(dt); }

  void AtlasAnimationTexture::Render(const float& dt) { Texture3D::Render(dt); }

  void AtlasAnimationTexture::PostRender(const float& dt) { Texture3D::PostRender(dt); }

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

  eResourceType AtlasAnimationTexture::GetResourceType() const { return RES_T_ATLAS_TEX; }

  void AtlasAnimationTexture::loadDerived(ComPtr<ID3D12Resource>& res)

  {
    const UINT num_atlases = static_cast<UINT>(m_atlases_.size());

    // Build the atlas from textures if no path is provided. (assuming that this has not been serialized before)
    if (GetPath().empty())
    {
      UINT width = 0, height = 0;

      // Find the largest atlas to set the 3D texture dimensions.
      for (UINT i = 0; i < num_atlases; ++i)
      {
        m_atlases_[i]->Load();
        width  = std::max(width, m_atlases_[i]->GetWidth());
        height = std::max(height, m_atlases_[i]->GetHeight());
      }

      LazyDescription
        (
         {
           .Alignment = 0,
           .Width = width,
           .Height = height,
           .DepthOrArraySize = static_cast<UINT16>(num_atlases),
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

  bool AtlasAnimationTexture::map(char* mapped)
  {
    const UINT num_atlases = static_cast<UINT>(m_atlases_.size());

    if (GetPath().empty())
    {
      // Merge all the atlases into the 3D texture.
      // Keep the atlases dimensions, progress from left to right, top to bottom
      // for matching with xml data.
      for (UINT i = 0; i < num_atlases; ++i)
      {
        ComPtr<ID3D12Resource> anim = m_atlases_[i]->GetRawResoruce();
        D3D12_BOX               box  = {0, 0, 0, m_atlases_[i]->GetWidth(), m_atlases_[i]->GetHeight(), 1};

        D3D12_TEXTURE_COPY_LOCATION dst
        {
          .pResource = GetRawResoruce(),
          .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
          .PlacedFootprint = {0, {DXGI_FORMAT_B8G8R8A8_UNORM, 0, i}}
        };

        D3D12_TEXTURE_COPY_LOCATION src
        {
          .pResource = anim.Get(),
          .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
          .SubresourceIndex = 0
        };

        GetD3Device().GetCommandList(COMMAND_LIST_COPY)->CopyTextureRegion
          (
           &dst,
           0,
           0,
           0,
           &src,
           &box
          );
      }

      return true;
    }

    return false;
  }
}
