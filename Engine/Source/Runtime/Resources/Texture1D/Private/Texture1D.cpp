#include "../Public/Texture1D.h"

namespace Engine::Resources
{
	void Texture1D::OnDeserialized()
	{
		Texture::OnDeserialized();
	}

	UINT64 Texture1D::GetWidth() const
	{
		return Texture::GetWidth();
	}

	void Texture1D::loadDerived(ComPtr<ID3D12Resource>& res)
	{
		const auto& gd = GetDescription();

		if (gd.Height || gd.DepthOrArraySize)
		{
			throw std::logic_error("1D Texture cannot have neither height nor depth");
		}

		if (GetPath().empty() && !(gd.Width))
		{
			throw std::logic_error("Hotloading texture should be define in width");
		}
	}

	void Texture1D::Unload_INTERNAL()
	{
		Texture::Unload_INTERNAL();
	}

	UINT Texture1D::GetHeight() const
	{
		return Texture::GetHeight();
	}

	UINT Texture1D::GetDepth() const
	{
		return Texture::GetDepth();
	}
}
