#include "pch.h"
#include "egTexture3D.h"

SERIALIZE_IMPL
(
   Engine::Resources::Texture3D,
   _ARTAG(_BSTSUPER(Texture))
)

namespace Engine::Resources
{
  UINT Texture3D::GetWidth() const { return Texture::GetWidth(); }

  UINT Texture3D::GetHeight() const { return Texture::GetHeight(); }

  UINT Texture3D::GetDepth() const { return Texture::GetDepth(); }

  void Texture3D::loadDerived(ComPtr<ID3D12Resource>& res)
  {
    const auto& gd = GetDescription();

    if (GetPath().empty() && !(gd.Width + gd.Height + gd.DepthOrArraySize))
    {
      throw std::logic_error("Hotloading texture should be define in width, height, depth");
    }
  }

  void Texture3D::Unload_INTERNAL()
  {
    Texture::Unload_INTERNAL();
    m_tex_.Reset();
  }
}
