#include "pch.h"
#include "egTexture3D.h"

SERIALIZER_ACCESS_IMPL
(
   Engine::Resources::Texture3D,
   _ARTAG(_BSTSUPER(Texture))
)

namespace Engine::Resources
{
  UINT Texture3D::GetWidth() const { return Texture::GetWidth(); }

  UINT Texture3D::GetHeight() const { return Texture::GetHeight(); }

  UINT Texture3D::GetDepth() const { return Texture::GetDepth(); }

  void Texture3D::loadDerived(ComPtr<ID3D11Resource>& res)
  {
    const auto& gd = GetDescription();

    if (GetPath().empty() && !(gd.Width + gd.Height + gd.Depth))
    {
      throw std::logic_error("Hotloading texture should be define in width, height, depth");
    }

    if (gd.ArraySize > 1) { throw std::logic_error("3D Texture cannot be array"); }

    if (!GetPath().empty())
    {
      res.As<ID3D11Texture3D>(&m_tex_);
    }
    else
    {
      const D3D11_TEXTURE3D_DESC desc
      {
        .Width = gd.Width,
        .Height = gd.Height,
        .Depth = gd.Depth,
        .MipLevels = gd.MipsLevel,
        .Format = gd.Format,
        .Usage = gd.Usage,
        .BindFlags = gd.BindFlags,
        .CPUAccessFlags = gd.CPUAccessFlags,
        .MiscFlags = gd.MiscFlags
      };

      GetD3Device().GetDevice()->CreateTexture3D(&desc, nullptr, m_tex_.ReleaseAndGetAddressOf());

      m_tex_.As<ID3D11Resource>(&res);
    }
  }

  void Texture3D::Unload_INTERNAL()
  {
    Texture::Unload_INTERNAL();
    m_tex_.Reset();
  }

  UINT Texture3D::GetArraySize() const { return Texture::GetArraySize(); }
}
