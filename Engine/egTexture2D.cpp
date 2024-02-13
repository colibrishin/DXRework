#include "pch.h"
#include "egTexture2D.h"

SERIALIZER_ACCESS_IMPL
(
 Engine::Resources::Texture2D,
 _ARTAG(_BSTSUPER(Texture))
)

namespace Engine::Resources
{
  UINT Texture2D::GetWidth() const { return Texture::GetWidth(); }

  UINT Texture2D::GetHeight() const { return Texture::GetHeight(); }

  UINT Texture2D::GetArraySize() const { return Texture::GetArraySize(); }

  void Texture2D::loadDerived(ComPtr<ID3D11Resource>& res)
  {
    const auto& gd = GetDescription();

    if (gd.Depth > 0) { throw std::logic_error("2D Texture cannot have depth, use array size instead"); }

    if (GetPath().empty() && !(gd.Width + gd.Height))
    {
      throw std::logic_error("Hotloading texture should be define in width, height");
    }

    if (!GetPath().empty())
    {
      res.As<ID3D11Texture2D>(&m_tex_);
    }
    else
    {
      const D3D11_TEXTURE2D_DESC desc
      {
        .Width = gd.Width,
        .Height = gd.Height,
        .MipLevels = gd.MipsLevel,
        .ArraySize = gd.ArraySize,
        .Format = gd.Format,
        .SampleDesc = gd.SampleDesc,
        .Usage = gd.Usage,
        .BindFlags = gd.BindFlags,
        .CPUAccessFlags = gd.CPUAccessFlags,
        .MiscFlags = gd.MiscFlags
      };

      GetD3Device().GetDevice()->CreateTexture2D(&desc, nullptr, m_tex_.ReleaseAndGetAddressOf());

      m_tex_.As<ID3D11Resource>(&res);
    }
  }

  void Texture2D::Unload_INTERNAL()
  {
    Texture::Unload_INTERNAL();
    m_tex_.Reset();
  }

  UINT Texture2D::GetDepth() const { return Texture::GetDepth(); }
}
