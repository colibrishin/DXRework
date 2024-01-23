#include "pch.h"
#include "egTexture1D.h"

namespace Engine::Resources
{
  void Texture1D::OnDeserialized() { Texture::OnDeserialized(); }

  void Texture1D::OnImGui() { Texture::OnImGui(); }

  UINT Texture1D::GetWidth() const { return Texture::GetWidth(); }

  void Texture1D::loadDerived(ComPtr<ID3D11Resource>& res)
  {
    const auto& gd = GetDescription();

    if (gd.Height || gd.Depth) { throw std::logic_error("1D Texture cannot have neither height nor depth"); }

    if (GetPath().empty() && !(gd.Width)) { throw std::logic_error("Hotloading texture should be define in width"); }

    if (!GetPath().empty()) { res.As<ID3D11Texture1D>(&m_tex_); }
    else
    {
      const D3D11_TEXTURE1D_DESC desc
      {
        .Width = gd.Width,
        .MipLevels = gd.Height,
        .ArraySize = gd.ArraySize,
        .Format = gd.Format,
        .Usage = gd.Usage,
        .BindFlags = gd.BindFlags,
        .CPUAccessFlags = gd.CPUAccessFlags,
        .MiscFlags = gd.MiscFlags
      };

      GetD3Device().GetDevice()->CreateTexture1D(&desc, nullptr, m_tex_.ReleaseAndGetAddressOf());

      m_tex_.As<ID3D11Resource>(&res);
    }
  }

  void Texture1D::Unload_INTERNAL()
  {
    Texture::Unload_INTERNAL();
    m_tex_.Reset();
  }

  UINT Texture1D::GetHeight() const { return Texture::GetHeight(); }

  UINT Texture1D::GetDepth() const { return Texture::GetDepth(); }

  UINT Texture1D::GetArraySize() const { return Texture::GetArraySize(); }
}
