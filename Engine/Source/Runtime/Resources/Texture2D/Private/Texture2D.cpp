#include "../Public/Texture2D.h"
#include "Source/Runtime/Managers/ResourceManager/Public/ResourceManager.hpp"

namespace Engine::Resources
{
	boost::shared_ptr<Texture2D> Texture2D::Create(
		const std::string& name, const std::filesystem::path& path, const GenericTextureDescription& desc
	)
	{
		if (const auto pcheck = Managers::ResourceManager::GetInstance().GetResourceByRawPath<Texture2D>(path).lock();
			const auto ncheck = Managers::ResourceManager::GetInstance().GetResource<Texture2D>(name).lock())
		{
			return ncheck;
		}
		const auto obj = boost::make_shared<Texture2D>(path, desc);
		Managers::ResourceManager::GetInstance().AddResource(name, obj);
		return obj;
	}

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
