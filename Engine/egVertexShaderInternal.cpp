#include "pch.h"
#include "egVertexShaderInternal.h"

#include "egRenderPipeline.h"

SERIALIZER_ACCESS_IMPL(
                       Engine::Graphics::VertexShaderInternal,
                       _ARTAG(_BSTSUPER(Shader<ID3D11VertexShader>))
                       _ARTAG(m_topology_))

namespace Engine::Graphics
{
    VertexShaderInternal::VertexShaderInternal(
        const EntityName&            name,
        const std::filesystem::path& path)
    : Shader<ID3D11VertexShader>(name, path) {}

    ID3D11InputLayout** VertexShaderInternal::GetInputLayout() {
        return m_input_layout_.GetAddressOf();
    }

    void              VertexShaderInternal::Render(const float& dt)
    {
        GetRenderPipeline().SetTopology(m_topology_);
        Shader<ID3D11VertexShader>::Render(dt);
    }

    VertexShaderInternal::VertexShaderInternal(): Shader<ID3D11VertexShader>("", {}) {}
} // namespace Engine::Graphic
