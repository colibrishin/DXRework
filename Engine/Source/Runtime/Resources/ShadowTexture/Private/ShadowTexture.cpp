#include "../Public/ShadowTexture.h"

#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.hpp"

namespace Engine::Resources
{
	boost::shared_ptr<ShadowTexture> ShadowTexture::Create(const std::string& name, const std::filesystem::path& path)
	{
		if (const auto pcheck = Engine::Managers::ResourceManager::GetInstance().GetResourceByRawPath<ShadowTexture>
					(path).lock();
			const auto ncheck = Engine::Managers::ResourceManager::GetInstance().GetResource<ShadowTexture>
					(name).lock())
		{
			return ncheck;
		}
		const auto obj = boost::make_shared<ShadowTexture>();
		Engine::Managers::ResourceManager::GetInstance().AddResource(name, obj);
		return obj;
	}

	void ShadowTexture::FixedUpdate(const float dt)
	{
		Texture2D::FixedUpdate(dt);
	}

	void ShadowTexture::Initialize()
	{
		Texture2D::Initialize();
	}

	void ShadowTexture::PostUpdate(const float dt)
	{
		Texture2D::PostUpdate(dt);
	}

	void ShadowTexture::PreUpdate(const float dt)
	{
		Texture2D::PreUpdate(dt);
	}

	void ShadowTexture::Update(const float dt)
	{
		Texture2D::Update(dt);
	}

	void ShadowTexture::OnSerialized()
	{
		Texture2D::OnSerialized();
	}

	void ShadowTexture::OnDeserialized()
	{
		Texture2D::OnDeserialized();
	}

	eResourceType ShadowTexture::GetResourceType() const
	{
		return RES_T_SHADOW_TEX;
	}

	UINT ShadowTexture::GetDepth() const
	{
		return Texture2D::GetDepth();
	}

	UINT ShadowTexture::GetHeight() const
	{
		return Texture2D::GetHeight();
	}

	UINT64 ShadowTexture::GetWidth() const
	{
		return Texture2D::GetWidth();
	}

	void ShadowTexture::Clear(const GraphicInterfaceContextPrimitive* context) const
	{
		g_graphic_interface.GetInterface().Clear(context, this, BIND_TYPE_DSV);
	}

	void ShadowTexture::Unload_INTERNAL()
	{
		Texture2D::Unload_INTERNAL();
	}
}
