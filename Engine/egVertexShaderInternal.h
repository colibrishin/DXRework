#pragma once
#include "egShader.hpp"
#include "egResourceManager.hpp"

namespace Engine::Graphics
{
    class VertexShaderInternal : public Graphics::Shader<ID3D11VertexShader>
    {
    public:
        VertexShaderInternal(
            const EntityName&            name,
            const std::filesystem::path& path);
        ~VertexShaderInternal() override = default;

        ID3D11InputLayout** GetInputLayout();

        void Render(const float& dt) override;

        RESOURCE_SELF_INFER_GETTER(VertexShaderInternal)

    protected:
        VertexShaderInternal();

    private:
        SERIALIZER_ACCESS

        D3D11_PRIMITIVE_TOPOLOGY m_topology_ = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

        ComPtr<ID3D11InputLayout> m_input_layout_ = nullptr;
    };
} // namespace Engine::Resources

BOOST_CLASS_EXPORT_KEY(Engine::Graphics::VertexShaderInternal)
