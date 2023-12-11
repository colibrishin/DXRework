#include "pch.hpp"
#include "egNormalMap.hpp"
#include "egRenderPipeline.hpp"

SERIALIZER_ACCESS_IMPL(Engine::Resources::NormalMap,
	_ARTAG(_BSTSUPER(Engine::Resources::Texture)))

namespace Engine::Resources
{
	void NormalMap::Initialize()
	{
		Texture::Initialize();
	}

	void NormalMap::PreUpdate(const float& dt)
	{
		Texture::PreUpdate(dt);
	}

	void NormalMap::Update(const float& dt)
	{
		Texture::Update(dt);
	}

	void NormalMap::PreRender(const float dt)
	{
		Texture::PreRender(dt);
	}

	void NormalMap::Render(const float dt)
	{
		GetRenderPipeline().BindResource(SR_NORMAL_MAP, m_texture_view_.Get());
	}

	TypeName NormalMap::GetVirtualTypeName() const
	{
		return typeid(NormalMap).name();
	}

	void NormalMap::Load_INTERNAL()
	{
		Texture::Load_INTERNAL();
	}

	void NormalMap::Unload_INTERNAL()
	{
		Texture::Unload_INTERNAL();
	}
}
