#include "../Public/Texture.h"

#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.hpp"

namespace Engine::Resources
{
	RESOURCE_SELF_INFER_GETTER_IMPL(Texture);

	Texture::Texture(std::filesystem::path path, const eTexType type, const GenericTextureDescription& description)
		: Resource(std::move(path), RES_T_TEX),
		  m_desc_(description),
		  m_type_(type) {}

	eTexType Texture::GetPrimitiveTextureType() const
	{
		return m_type_;
	}

	const GenericTextureDescription& Texture::GetDescription() const
	{
		return m_desc_;
	}

	bool Texture::IsHotload() const
	{
		return GetPath().empty();
	}
	
	Texture::Texture()
		: Resource("", RES_T_TEX), m_desc_({}), m_type_(TEX_TYPE_2D)
	{
	}

	UINT64 Texture::GetWidth() const
	{
		return m_desc_.Width;
	}

	UINT Texture::GetHeight() const
	{
		return m_desc_.Height;
	}

	UINT Texture::GetDepth() const
	{
		return m_desc_.DepthOrArraySize;
	}

	void Texture::Initialize() { }

	void Texture::PreUpdate(const float& dt) {}

	void Texture::Update(const float& dt) {}

	void Texture::PostUpdate(const float& dt) {}

	void Texture::Load_INTERNAL()
	{
		if (!GetPath().empty())
		{
			m_primitive_texture_->LoadFromFile(GetSharedPtr<Texture>(), GetPath());
		}
		else
		{
			m_primitive_texture_->Generate(GetSharedPtr<Texture>());
			Map();
		}
	}

	void Texture::UpdateDescription(const GenericTextureDescription& description)
	{
		if (!IsLoaded())
		{
			m_desc_ = description;
		}
	}

	PrimitiveTexture* Texture::GetPrimitiveTexture() const
	{
		return m_primitive_texture_.get();
	}

	void Texture::Unload_INTERNAL()
	{
		m_primitive_texture_.reset();
	}

	void Texture::FixedUpdate(const float& dt) {}

	void Texture::OnSerialized()
	{
		Resource::OnSerialized();

		const auto                  name       = GetName();
		const std::filesystem::path folder     = GetPrettyTypeName();
		const std::filesystem::path filename   = name + ".dds";
		const std::filesystem::path final_path = folder / filename;

		if (!IsLoaded())
		{
			Load();
		}

		m_primitive_texture_->SaveAsFile(final_path);
	}

	void Texture::OnDeserialized()
	{
		Resource::OnDeserialized();
	}

	eResourceType Texture::GetResourceType() const
	{
		return Resource::GetResourceType();
	}
} // namespace Engine::Resources

namespace Engine 
{
	void PrimitiveTexture::UpdateDescription(
		const Weak<Resources::Texture>& texture,
		const GenericTextureDescription& description)
	{
		if (const Strong<Resources::Texture>& tex = texture.lock())
		{
			tex->UpdateDescription(description);
		}

		m_description_ = description;
	}

	void* PrimitiveTexture::GetPrimitiveTexture() const
	{
		return m_texture_;
	}

	void PrimitiveTexture::SetPrimitiveTexture(void* texture)
	{
		if (texture)
		{
			m_texture_ = texture;
		}
	}

	const GenericTextureDescription& PrimitiveTexture::GetDescription() const
	{
		return m_description_;
	}
}