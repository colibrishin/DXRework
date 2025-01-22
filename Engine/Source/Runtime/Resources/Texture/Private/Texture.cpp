#include "../Public/Texture.h"
#include "Texture.generated.h"

#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"

namespace Engine::Resources
{
	Texture::Texture(std::filesystem::path path, const eTexType type, const GenericTextureDescription& description)
		: Resource(std::move(path)),
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
		: Resource(""), m_desc_({}), m_type_(TEX_TYPE_2D)
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

	void Texture::PreUpdate(const float dt) {}

	void Texture::Update(const float dt) {}

	void Texture::PostUpdate(const float dt) {}

	void Texture::Load_INTERNAL()
	{
		m_primitive_texture_ = Unique<PrimitiveTexture>(GraphicInterfaceAccessor::GetInterface().GetNewPrimitiveTexture());

		if (!GetPath().empty())
		{
			m_primitive_texture_->LoadFromFile(this, GetPath());
		}
		else
		{
			m_primitive_texture_->Generate(this);
			m_desc_ = m_primitive_texture_->GetDescription();
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

	void Texture::FixedUpdate(const float dt) {}

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
} // namespace Engine::Resources