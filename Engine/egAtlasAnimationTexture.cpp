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

  void AtlasAnimationTexture::PreRender(const float& dt) {}

  void AtlasAnimationTexture::Render(const float& dt)
  {
    BindAs(D3D11_BIND_SHADER_RESOURCE, RESERVED_ATLAS, 0, SHADER_PIXEL);
    Texture3D::Render(dt);
  }

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

  void AtlasAnimationTexture::loadDerived(ComPtr<ID3D11Resource>& res)
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
           .Width = width,
           .Height = height,
           .Depth = num_atlases,
           .ArraySize = 1,
           .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
           .CPUAccessFlags = 0,
           .BindFlags = D3D11_BIND_SHADER_RESOURCE,
           .MipsLevel = 1,
           .MiscFlags = 0,
           .Usage = D3D11_USAGE_DEFAULT,
           .SampleDesc = {1, 0}
         }
        );
    }

    Texture3D::loadDerived(res);

    if (GetPath().empty())
    {
      ComPtr<ID3D11Texture3D> tex;
      DX::ThrowIfFailed(res.As(&tex));

      // Merge all the atlases into the 3D texture.
      // Keep the atlases dimensions, progress from left to right, top to bottom
      // for matching with xml data.
      for (UINT i = 0; i < num_atlases; ++i)
      {
        ComPtr<ID3D11Texture2D> anim = m_atlases_[i]->As<ID3D11Texture2D>();
        D3D11_BOX               box  = {0, 0, 0, m_atlases_[i]->GetWidth(), m_atlases_[i]->GetHeight(), 1};

        GetD3Device().GetContext()->CopySubresourceRegion
          (
           tex.Get(),
           0,
           0,
           0,
           i,
           anim.Get(),
           0,
           &box
          );
      }
    }
  }
}
