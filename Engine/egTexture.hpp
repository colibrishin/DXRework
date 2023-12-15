#pragma once
#include "egD3Device.hpp"
#include "egResource.hpp"

namespace Engine::Resources
{
    class Texture : public Abstract::Resource
    {
    public:
        explicit Texture(std::filesystem::path path)
        : Resource(std::move(path), RESOURCE_PRIORITY_TEXTURE),
          m_texture_desc_()
        {
            Texture::Initialize();
        }

        ~Texture() override;

        void     Initialize() override;
        void     PreUpdate(const float& dt) override;
        void     Update(const float& dt) override;
        void     PreRender(const float& dt) override;
        void     Render(const float& dt) override;
        void     PostRender(const float& dt) override;
        void     Load_INTERNAL() override;
        void     Unload_INTERNAL() override;
        void     FixedUpdate(const float& dt) override;
        TypeName GetVirtualTypeName() const override;

        UINT GetWidth() const
        {
            return m_texture_desc_.Width;
        }

        UINT GetHeight() const
        {
            return m_texture_desc_.Height;
        }

    protected:
        Texture()
        : Resource("", RESOURCE_PRIORITY_TEXTURE),
          m_texture_desc_() {}

        ComPtr<ID3D11ShaderResourceView> m_texture_view_;

    private:
        SERIALIZER_ACCESS

        D3D11_TEXTURE2D_DESC m_texture_desc_;
    };
} // namespace Engine::Resources

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Texture)
