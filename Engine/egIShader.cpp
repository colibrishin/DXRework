#include "pch.hpp"
#include "egIShader.hpp"

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

    void IShader::Render(const float& dt)
    {
        GetRenderPipeline().SetShader(this);
    }
} // namespace Engine::Graphic
