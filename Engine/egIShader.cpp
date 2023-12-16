#include "pch.h"
#include "egIShader.h"

#include "egManagerHelper.hpp"

SERIALIZER_ACCESS_IMPL(
                       Engine::Graphic::IShader,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Resource))
                       _ARTAG(m_type_));

namespace Engine::Graphic
{
    IShader::IShader(const EntityName& name, const std::filesystem::path& path)
    : Resource(path, RESOURCE_PRIORITY_SHADER)
    {
        SetName(name);
    }

    ID3D11Buffer* IShader::GetBuffer() const
    {
        return m_buffer_.Get();
    }

    eShaderType IShader::GetType() const
    {
        return m_type_;
    }

    void      IShader::Render(const float& dt)
    {
        GetRenderPipeline().SetShader(this);
    }
} // namespace Engine::Graphic
