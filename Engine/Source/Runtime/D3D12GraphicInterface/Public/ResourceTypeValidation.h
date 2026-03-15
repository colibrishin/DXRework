#pragma once

// Runtime type validation at graphic API boundaries: assert derived type before casting
// so that e.g. a Sound cannot be passed where a Texture is expected.

#include "IGraphicAPI.h"
#include "Texture.h"
#include "Mesh.h"
#include "ShaderBase.h"
#include "Shader.h"
#include "ComputeShader.h"
#include "Font.h"

#if CFG_RAYTRACING
#include "RaytracingShader.h"
#endif

namespace Engine::D3D12
{
    inline void ExpectTexture( const Abstracts::Resource* resource )
    {
        assert( resource && resource->IsDerivedOf( Resources::Texture::StaticTypeHash() ) );
        (void)resource;
    }

    inline void ExpectMesh( const Abstracts::Resource* resource )
    {
        assert( resource && resource->IsDerivedOf( Resources::Mesh::StaticTypeHash() ) );
        (void)resource;
    }

    inline void ExpectShaderBase( const Abstracts::Resource* resource )
    {
        assert( resource && resource->IsDerivedOf( Resources::ShaderBase::StaticTypeHash() ) );
        (void)resource;
    }

    inline void ExpectShader( const Abstracts::Resource* resource )
    {
        assert( resource && resource->IsDerivedOf( Resources::Shader::StaticTypeHash() ) );
        (void)resource;
    }

    inline void ExpectComputeShader( const Abstracts::Resource* resource )
    {
        assert( resource && resource->IsDerivedOf( Resources::ComputeShader::StaticTypeHash() ) );
        (void)resource;
    }

    inline void ExpectFont( const Abstracts::Resource* resource )
    {
        assert( resource && resource->IsDerivedOf( Resources::Font::StaticTypeHash() ) );
        (void)resource;
    }

#if CFG_RAYTRACING
    inline void ExpectRaytracingShader( const Abstracts::Resource* resource )
    {
        assert( resource && resource->IsDerivedOf( Resources::RaytracingShader::StaticTypeHash() ) );
        (void)resource;
    }
#endif
}
