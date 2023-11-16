#pragma once
#include "egD3Device.hpp"
#include "egResource.hpp"

namespace Engine::Resources
{
	class Texture : public Abstract::Resource
	{
	public:
		explicit Texture(std::filesystem::path path) : Resource(std::move(path), RESOURCE_PRIORITY_TEXTURE), m_texture_desc_()
		{
			Texture::Initialize();
		}

		~Texture() override = default;

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;
		void FixedUpdate(const float& dt) override;

		UINT GetWidth() const { return m_texture_desc_.Width; }
		UINT GetHeight() const { return m_texture_desc_.Height; }

	protected:
		ComPtr<ID3D11ShaderResourceView> m_texture_view_;

	private:
		ComPtr<ID3D11Resource> m_texture_;

		D3D11_TEXTURE2D_DESC m_texture_desc_;
	};

	inline void Texture::Initialize()
	{
	}

	inline void Texture::PreUpdate(const float& dt)
	{
	}

	inline void Texture::Update(const float& dt)
	{
	}

	inline void Texture::PreRender(const float dt)
	{
	}

	inline void Texture::Render(const float dt)
	{
		GetRenderPipeline().BindResource(SR_TEXTURE, m_texture_view_.Get());
	}

	inline void Texture::Load_INTERNAL()
	{
		GetD3Device().CreateTextureFromFile(absolute(GetPath()), m_texture_.ReleaseAndGetAddressOf(),
		                                         m_texture_view_.ReleaseAndGetAddressOf());

		ComPtr<ID3D11Texture2D> texture;
		m_texture_.As<ID3D11Texture2D>(&texture);

		texture->GetDesc(&m_texture_desc_);
	}

	inline void Texture::Unload_INTERNAL()
	{
		m_texture_view_->Release();
		m_texture_->Release();
		m_texture_desc_ = {};
	}

	inline void Texture::FixedUpdate(const float& dt)
	{
	}
}
