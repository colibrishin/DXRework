#include "pch.hpp"
#include "egTexture.hpp"
#include "egRenderPipeline.hpp"
#include "egD3Device.hpp"

SERIALIZER_ACCESS_IMPL(Engine::Resources::Texture,
	_ARTAG(_BSTSUPER(Resource)))

namespace Engine::Resources
{
	void Texture::Initialize()
	{
	}

	void Texture::PreUpdate(const float& dt)
	{
	}

	void Texture::Update(const float& dt)
	{
	}

	void Texture::PreRender(const float dt)
	{
	}

	void Texture::Render(const float dt)
	{
		GetRenderPipeline().BindResource(SR_TEXTURE, m_texture_view_.Get());
	}

	void Texture::Load_INTERNAL()
	{
		GetD3Device().CreateTextureFromFile(absolute(GetPath()), m_texture_.ReleaseAndGetAddressOf(),
		                                         m_texture_view_.ReleaseAndGetAddressOf());

		ComPtr<ID3D11Texture2D> texture;
		m_texture_.As<ID3D11Texture2D>(&texture);

		texture->GetDesc(&m_texture_desc_);
	}

	void Texture::Unload_INTERNAL()
	{
		m_texture_view_->Release();
		m_texture_->Release();
		m_texture_desc_ = {};
	}

	void Texture::FixedUpdate(const float& dt)
	{
	}
}
