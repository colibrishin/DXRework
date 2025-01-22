#include "ShadowRenderTarget.h"
#include "ShadowRenderTarget.generated.h"

#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"

namespace Engine::Resources
{
	void ShadowRenderTarget::FixedUpdate(const float dt)
	{
		Texture2D::FixedUpdate(dt);
	}

	void ShadowRenderTarget::Initialize()
	{
		Texture2D::Initialize();
	}

	void ShadowRenderTarget::PostUpdate(const float dt)
	{
		Texture2D::PostUpdate(dt);
	}

	void ShadowRenderTarget::PreUpdate(const float dt)
	{
		Texture2D::PreUpdate(dt);
	}

	void ShadowRenderTarget::Update(const float dt)
	{
		Texture2D::Update(dt);
	}

	void ShadowRenderTarget::OnSerialized()
	{
		Texture2D::OnSerialized();
	}

	void ShadowRenderTarget::OnDeserialized()
	{
		Texture2D::OnDeserialized();
	}

	UINT ShadowRenderTarget::GetDepth() const
	{
		return Texture2D::GetDepth();
	}

	UINT ShadowRenderTarget::GetHeight() const
	{
		return Texture2D::GetHeight();
	}

	UINT64 ShadowRenderTarget::GetWidth() const
	{
		return Texture2D::GetWidth();
	}

	void ShadowRenderTarget::Clear(const GraphicInterfaceContextPrimitive* context) const
	{
		GraphicInterfaceAccessor::GetInterface().Clear(context, this, BIND_TYPE_RTV);
	}

	void ShadowRenderTarget::Unload_INTERNAL()
	{
		Texture2D::Unload_INTERNAL();
	}
}
