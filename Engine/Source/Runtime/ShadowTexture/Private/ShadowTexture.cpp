#include "ShadowTexture.h"
#include "ShadowTexture.generated.h"

#include "ResourceManager.h"

namespace Engine::Resources
{
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
		GraphicInterfaceAccessor::GetInterface().Clear(context, this, BIND_TYPE_DSV);
	}

	void ShadowTexture::Unload_INTERNAL()
	{
		Texture2D::Unload_INTERNAL();
	}
}
