#pragma once
#include <filesystem>

#include "egCommon.hpp"
#include "egResource.h"

namespace Engine::Graphics
{
    using Microsoft::WRL::ComPtr;

    class IShader : public Abstract::Resource
    {
    public:
        IShader(const EntityName& name, const std::filesystem::path& path);
        ~IShader() override = default;

        ID3D11Buffer* GetBuffer() const;

        void Render(const float& dt) override;

        eShaderType GetType() const;

    protected:
        virtual void SetShaderType() = 0;
        eShaderType  m_type_;

    private:
        SERIALIZER_ACCESS

        ComPtr<ID3D11Buffer> m_buffer_;
    };
} // namespace Engine::Graphic

BOOST_CLASS_EXPORT_KEY(Engine::Graphics::IShader)
