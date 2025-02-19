#include "ShaderBase.h"
#include "ShaderBase.generated.h"

Engine::Resources::ShaderBase::ShaderBase( const std::filesystem::path &path, const eShaderDomain domain )
    : Resource( path ), m_domain_( domain )
{}

Engine::Resources::ShaderBase::ShaderBase( const ShaderBase &other ) : Resource( other )
{
    m_domain_ = other.m_domain_;
}

Engine::Resources::ShaderBase &Engine::Resources::ShaderBase::operator=( const ShaderBase &other )
{
    m_domain_ = other.m_domain_;
    return *this;
}

Engine::eShaderDomain Engine::Resources::ShaderBase::GetShaderDomain() const
{
    return m_domain_;
}

Engine::Resources::ShaderBase::ShaderBase() : Resource( "" ), m_domain_()
{}