#pragma once
#include <directx/d3d12.h>

#include "TypeLibrary.h"

namespace Engine
{
    // eShaderSamplerAddress does not share D3D12's values (CLAMP is 1 in the engine, WRAP in D3D12).
    inline D3D12_TEXTURE_ADDRESS_MODE ToD3D12AddressMode(const eShaderSamplerAddress address)
    {
        switch (address)
        {
        case SHADER_SAMPLER_CLAMP:       return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case SHADER_SAMPLER_WRAP:        return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case SHADER_SAMPLER_MIRROR:      return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case SHADER_SAMPLER_BORDER:      return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        case SHADER_SAMPLER_MIRROR_ONCE: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
        default:                         return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        }
    }
}
