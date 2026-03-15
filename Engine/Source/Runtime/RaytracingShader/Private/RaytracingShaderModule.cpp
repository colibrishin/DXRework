#include "RaytracingShaderModule.h"

#include "RaytracingShader.h"

MODULE_IMPL(Engine::RaytracingShaderModule, RaytracingShader)

bool Engine::RaytracingShaderModule::InitializeImpl()
{
    Resources::RaytracingShader::Create(
        "raytracing", "./raytracing.hlsl", SHADER_DOMAIN_OPAQUE,
        std::array{true, false, true, true}, L"hitgroup0", SAMPLER_FILTER_MIN_MAG_MIP_LINEAR,
        SHADER_SAMPLER_CLAMP, SHADER_SAMPLER_LESS_EQUAL);

    return true;
}

bool Engine::RaytracingShaderModule::ShutdownImpl()
{
    return true;
}

bool Engine::RaytracingShaderModule::DynamicLoadable()
{
    return true;
}
