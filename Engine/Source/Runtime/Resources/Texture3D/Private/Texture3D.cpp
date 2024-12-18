#include "../Public/Texture3D.h"

namespace Engine::Resources
{
	UINT64 Texture3D::GetWidth() const
	{
		return Texture::GetWidth();
	}

	UINT Texture3D::GetHeight() const
	{
		return Texture::GetHeight();
	}

	UINT Texture3D::GetDepth() const
	{
		return Texture::GetDepth();
	}

	void Texture3D::Load_INTERNAL()
	{
		const auto& gd = GetDescription();

		if (GetPath().empty() && !(gd.Width + gd.Height + gd.DepthOrArraySize))
		{
			throw std::logic_error("Hotloading texture should be define in width, height, depth");
		}

		Texture::Load_INTERNAL();
	}

	void Texture3D::Unload_INTERNAL()
	{
		Texture::Unload_INTERNAL();
	}
}
