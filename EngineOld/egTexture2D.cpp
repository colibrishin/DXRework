#include "pch.h"
#include "egTexture2D.h"

SERIALIZE_IMPL
(
 Engine::Resources::Texture2D,
 _ARTAG(_BSTSUPER(Texture))
)

namespace Engine::Resources
{
	UINT64 Texture2D::GetWidth() const
	{
		return Texture::GetWidth();
	}

	UINT Texture2D::GetHeight() const
	{
		return Texture::GetHeight();
	}

	void Texture2D::loadDerived(ComPtr<ID3D12Resource>& res)
	{
		const auto& gd = GetDescription();

		if (GetPath().empty() && !(gd.Width + gd.Height))
		{
			throw std::logic_error("Hotloading texture should be define in width, height");
		}
	}

	void Texture2D::Unload_INTERNAL()
	{
		Texture::Unload_INTERNAL();
	}

	UINT Texture2D::GetDepth() const
	{
		return Texture::GetDepth();
	}
}
