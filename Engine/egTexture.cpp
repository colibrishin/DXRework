#include "pch.hpp"
#include "egTexture.hpp"
#include "egD3Device.hpp"
#include "egRenderPipeline.hpp"

SERIALIZER_ACCESS_IMPL(Engine::Resources::Texture, _ARTAG(_BSTSUPER(Resource)))

namespace Engine::Resources
{
    Texture::~Texture() {}

    void Texture::Initialize() {}

    void Texture::PreUpdate(const float& dt) {}

    void Texture::Update(const float& dt) {}

    void Texture::PreRender(const float& dt) {}

    void Texture::Render(const float& dt)
    {
        GetRenderPipeline().BindResource(SR_TEXTURE, m_texture_view_.Get());
    }

    void Texture::PostRender(const float& dt) {}

    void Texture::Load_INTERNAL()
    {
        ComPtr<ID3D11Resource>  resource;
        ComPtr<ID3D11Texture2D> texture;

        GetD3Device().CreateTextureFromFile(
                                            absolute(GetPath()),
                                            resource.ReleaseAndGetAddressOf(),
                                            m_texture_view_.ReleaseAndGetAddressOf());

        resource.As<ID3D11Texture2D>(&texture);
        texture->GetDesc(&m_texture_desc_);
    }

    void Texture::Unload_INTERNAL()
    {
        m_texture_view_->Release();
        m_texture_view_.Reset();
        m_texture_desc_ = {};
    }

    void Texture::FixedUpdate(const float& dt) {}

    TypeName Texture::GetVirtualTypeName() const
    {
        return typeid(Texture).name();
    }
} // namespace Engine::Resources
