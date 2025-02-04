#include "RaytracingShaderModule.h"
#include "RaytracingShaderModule.generated.h"
#include "ModuleManager/Public/ModuleManager.h"

#include "RaytracingShader.h"


MODULE_IMPL(Engine::RaytracingShaderModule, RaytracingShader)

void Engine::RaytracingShaderModule::Initialize()
{
    Resources::RaytracingShader::Create(
        "raytracing", "./raytracing.hlsl", SHADER_DOMAIN_OPAQUE,
        std::array{true, false, true, true}, L"hitgroup0", SAMPLER_FILTER_MIN_MAG_MIP_LINEAR,
        SHADER_SAMPLER_CLAMP, SHADER_SAMPLER_LESS_EQUAL);
}

void Engine::RaytracingShaderModule::Shutdown()
{
    
}

bool Engine::RaytracingShaderModule::DynamicLoadable()
{
    return true;
}
