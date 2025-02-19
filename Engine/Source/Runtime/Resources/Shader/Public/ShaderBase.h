#pragma once
#include "GraphicInterface.h"
#include "Resource/Public/Resource.h"
#include "ResourceManager/Public/ResourceManager.h"

#include "ShaderBase.generated.h"

namespace Engine::Resources
{
    ECLASS( resource, abstract, serialize )
    class ENGINE_SHADER_API ShaderBase : public Abstracts::Resource
    {
        GENERATE_BODY

        ShaderBase( const std::filesystem::path &path, const eShaderDomain domain );

        ShaderBase( const ShaderBase & );
        ShaderBase &operator=( const ShaderBase & );

        [[nodiscard]] eShaderDomain GetShaderDomain() const;

        [[nodiscard]] virtual PrimitiveShaderBase &GetPrimitive() const = 0;

    protected:
        ShaderBase();

        EPROPERTY()
        eShaderDomain m_domain_;
    };
} // namespace Engine::Resources