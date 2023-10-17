#pragma once
#include "egD3Device.hpp"
#include "egResource.hpp"

namespace Engine::Resources
{
	class Texture : public Abstract::Resource
	{
	public:
		explicit Texture(std::filesystem::path path) : Resource(std::move(path)), m_texture_desc_()
		{
			Texture::Initialize();
		}

		virtual ~Texture() override = default;

		void Initialize() override;
		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;
		void Load() override;
		void Unload() override;

		UINT GetWidth() const { return m_texture_desc_.Width; }
		UINT GetHeight() const { return m_texture_desc_.Height; }

	private:
		ComPtr<ID3D11Resource> m_texture_;
		ComPtr<ID3D11ShaderResourceView> m_texture_view_;

		D3D11_TEXTURE2D_DESC m_texture_desc_;
	};

	inline void Texture::Initialize()
	{
		Load();
	}

	inline void Texture::PreUpdate()
	{
	}

	inline void Texture::Update()
	{
	}

	inline void Texture::PreRender()
	{
	}

	inline void Texture::Render()
	{
		Graphic::RenderPipeline::BindTexture(m_texture_view_.Get());
	}

	inline void Texture::Load()
	{
		Graphic::D3Device::CreateTextureFromFile(std::filesystem::absolute(GetPath()), m_texture_.ReleaseAndGetAddressOf(), m_texture_view_.ReleaseAndGetAddressOf());

		ComPtr<ID3D11Texture2D> texture;
		m_texture_.As<ID3D11Texture2D>(&texture);

		texture->GetDesc(&m_texture_desc_);
	}

	inline void Texture::Unload()
	{
		m_texture_view_->Release();
		m_texture_->Release();
		m_texture_desc_ = {};
	}
}
