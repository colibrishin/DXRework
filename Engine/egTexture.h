#pragma once
#include "egD3Device.hpp"
#include "egResource.h"

namespace Engine::Resources
{
    class Texture : public Abstract::Resource
    {
    public:
        RESOURCE_T(RES_T_TEX)

        explicit Texture(std::filesystem::path path);

        ~Texture() override;

        void Initialize() override;
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void PostUpdate(const float& dt) override;
        void Load_INTERNAL() override;
        void Unload_INTERNAL() override;
        void FixedUpdate(const float& dt) override;

        UINT GetWidth() const;
        UINT GetHeight() const;

        void SetSlot(eTexBindSlot slot, UINT slot_offset);

        RESOURCE_SELF_INFER_GETTER(Texture)
        RESOURCE_SELF_INFER_CREATE(Texture)

    protected:
        Texture();

        ComPtr<ID3D11ShaderResourceView> m_texture_view_;

    private:
        SERIALIZER_ACCESS

        D3D11_TEXTURE2D_DESC m_texture_desc_;
        UINT m_bound_slot_;
    };
} // namespace Engine::Resources

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Texture)
