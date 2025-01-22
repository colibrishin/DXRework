#include "../Public/AtlasAnimationTexture.h"

#include "Source/Runtime/Resources/Texture2D/Public/Texture2D.h"
#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.hpp"

namespace Engine::Resources
{
	AtlasAnimationTexture::AtlasAnimationTexture(
		const std::filesystem::path& path, const std::vector<Strong<Texture2D>>& atlases
	)
		: Texture3D(path, {}),
		  m_atlases_(atlases) {}

	void AtlasAnimationTexture::PreUpdate(const float& dt) {}

	void AtlasAnimationTexture::Update(const float& dt) {}

	void AtlasAnimationTexture::FixedUpdate(const float& dt) {}

	void AtlasAnimationTexture::PostUpdate(const float& dt) {}

	void AtlasAnimationTexture::OnSerialized()
	{
		Texture3D::OnSerialized();
	}

	void AtlasAnimationTexture::OnDeserialized()
	{
		Texture3D::OnDeserialized();
	}

	eResourceType AtlasAnimationTexture::GetResourceType() const
	{
		return RES_T_ATLAS_TEX;
	}

	boost::shared_ptr<AtlasAnimationTexture> AtlasAnimationTexture::Create(
		const std::string& name, const std::filesystem::path& path, const std::vector<Strong<Texture2D>>& atlases
	)
	{
		if (const auto ncheck = Managers::ResourceManager::GetInstance().GetResource<AtlasAnimationTexture>(name).lock())
		{
			return ncheck;
		}

		if (const auto pcheck = Managers::ResourceManager::GetInstance().GetResourceByMetadataPath<AtlasAnimationTexture>(path).lock())
		{
			return pcheck;
		}

		const auto obj = boost::make_shared<AtlasAnimationTexture>(path, atlases);
		Managers::ResourceManager::GetInstance().AddResource(name, obj);

		// Sort atlases by order of name
		std::ranges::sort
				(
				 obj->m_atlases_,
				 [](const Strong<Texture2D>& a, const Strong<Texture2D>& b)
				 {
					 return a->GetName() < b->GetName();
				 }
				);

		return obj;
	}

	void AtlasAnimationTexture::Load_INTERNAL()
	{
		if (m_atlases_.size() > std::numeric_limits<UINT16>::max())
		{
			OutputDebugStringW(L"Warning: atlas textures are given more than limit 65535\n");
		}

		const UINT16 num_atlases = static_cast<UINT16>(m_atlases_.size());

		// Build the atlas from textures if no path is provided. (assuming that this has not been serialized before)
		if (GetPath().empty())
		{
			UINT64 width  = 0;
			UINT   height = 0;

			// Find the largest atlas to set the 3D texture dimensions.
			for (UINT16 i = 0; i < num_atlases; ++i)
			{
				m_atlases_[i]->Load();
				width  = std::max(width, m_atlases_[i]->GetWidth());
				height = std::max(height, m_atlases_[i]->GetHeight());
			}

			GenericTextureDescription new_desc =
			{
				.Alignment = 0,
				.Width = width,
				.Height = height,
				.DepthOrArraySize = num_atlases,
				.Format = TEX_FORMAT_B8G8R8A8_UNORM,
				.Flags = RESOURCE_FLAG_NONE,
				.MipsLevel = 1,
				.Layout = TEX_LAYOUT_UNKNOWN,
				.SampleDesc = {1, 0}
			};

			GetPrimitiveTexture()->UpdateDescription(GetSharedPtr<AtlasAnimationTexture>(), new_desc);
		}

		Texture3D::Load_INTERNAL();
	}

	void AtlasAnimationTexture::Map()
	{
		// See DepthOrArraySize for type.
		const UINT16 num_atlases = static_cast<UINT16>(m_atlases_.size());
		PrimitiveTexture* atlas_tex = GetPrimitiveTexture();
		TextureMappingTask& mapping_task = atlas_tex->GetMappingTask();

		if (GetPath().empty())
		{
			// Merge all the atlases into the 3D texture.
			// Keep the atlases dimensions, progress from left to right, top to bottom
			// for matching with xml data.
			for (UINT16 i = 0; i < num_atlases; ++i)
			{
				mapping_task.Map(
					atlas_tex,
					m_atlases_[i]->GetPrimitiveTexture(),
					m_atlases_[i]->GetWidth(),
					m_atlases_[i]->GetHeight(),
					1,
					0,
					0,
					i);
			}
		}
	}
}
