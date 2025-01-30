#include "RaytracingShader.h"

Engine::Resources::RaytracingShader::RaytracingShader(
    const std::filesystem::path& path,
    const std::array<bool, 4>&   has_export,
    const std::wstring_view      hit_group_name,
    const eSamplerFilter         sampler_filter,
    const eShaderSamplerAddress  sampler_addr_mode,
    const eShaderSamplerFunction sampler_func,
    const std::array<size_t, 3>& shader_record_sizes) :
Resource(path),
m_has_export_(has_export),
m_shader_record_sizes_(shader_record_sizes),
m_hit_group_name_(hit_group_name),
m_sampler_filter_(sampler_filter),
m_sampler_addr_mode_(sampler_addr_mode),
m_sampler_func_(sampler_func) { }

void Engine::Resources::RaytracingShader::PreUpdate(const float dt) { }

void Engine::Resources::RaytracingShader::Update(const float dt) { }

void Engine::Resources::RaytracingShader::PostUpdate(const float dt) { }

void Engine::Resources::RaytracingShader::FixedUpdate(const float dt) { }

void       Engine::Resources::RaytracingShader::OnSerialized() { }

const std::array<bool, 4>& Engine::Resources::RaytracingShader::GetHasExport() const
{
    return m_has_export_;
}

const std::array<size_t, 3>& Engine::Resources::RaytracingShader::GetShaderRecordSizes() const
{
    return m_shader_record_sizes_;
}

std::wstring_view Engine::Resources::RaytracingShader::GetHitGroupName() const
{
    return m_hit_group_name_;
}

Engine::eSamplerFilter Engine::Resources::RaytracingShader::GetSamplerFilter() const
{
    return m_sampler_filter_;
}

Engine::eShaderSamplerAddress Engine::Resources::RaytracingShader::GetSamplerAddressMode() const
{
    return m_sampler_addr_mode_;
}

Engine::eShaderSamplerFunction Engine::Resources::RaytracingShader::GetSamplerFunction() const
{
    return m_sampler_func_;
}

Engine::RaytracingPrimitiveShader* Engine::Resources::RaytracingShader::GetPrimitive() const
{
    return m_primitive_shader_.get();
}

Engine::Resources::RaytracingShader::RaytracingShader()
    : Resource(),
      m_sampler_filter_(),
      m_sampler_addr_mode_(),
      m_sampler_func_() {}

void Engine::Resources::RaytracingShader::Load_INTERNAL()
{
    RaytracingExtensionInterface& rgi = GraphicInterfaceAccessor::GetRaytracingInterface();
    m_primitive_shader_ = std::unique_ptr<decltype(m_primitive_shader_)::element_type>(rgi.GetNewRaytracingShader());
    m_primitive_shader_->Generate(this, rgi.GetRaytracingNativePipeline());
}

void Engine::Resources::RaytracingShader::Unload_INTERNAL()
{
    m_primitive_shader_.reset();
}
