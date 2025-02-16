#pragma once
#include "ComputeShader.h"
#include "TypeLibrary/Public/TypeLibrary.h"

#include "ShadowIntensityComputeShader.generated.h"

class IntensityPositionTexture;
class IntensityTexture;

struct LightTableSB
{
    CLIENT_SB_UAV_T( static_cast<Engine::eClientSBUAVType>(7) )

    Engine::OffsetT<int> lightTable[ CFG_MAX_DIRECTIONAL_LIGHT ];
    Vector4              min[ CFG_MAX_DIRECTIONAL_LIGHT ];
    Vector4              max[ CFG_MAX_DIRECTIONAL_LIGHT ];

    LightTableSB()
    {
        for ( int i = 0; i < CFG_MAX_DIRECTIONAL_LIGHT; ++i )
        {
            lightTable[ i ] = 0;
            min[ i ]        = Vector4( FLT_MAX );
            max[ i ]        = Vector4( FLT_MIN );
        }
    }
};


ECLASS(resource=client, serialize)
class ENGINE_CLIENT_API ShadowIntensityComputeShader : public Engine::Resources::ComputeShader
{
    GENERATE_BODY
public:
    constexpr static size_t target_light_slot = 0;

    void SetLightTable(const Engine::Weak<Engine::StructuredBufferTypeProxy<LightTableSB>>& light_table_ptr)
    {
        m_light_table_ptr_ = light_table_ptr;
    }

    void SetIntersectionTexture(const Engine::Weak<IntensityTexture>& texs)
    {
        if (const Engine::Strong<IntensityTexture>& locked = texs.lock())
        {
            m_intersection_texture_ = locked;   
        }
    }

    void SetPositionTexture(const Engine::Weak<IntensityPositionTexture>& texs)
    {
        if (const Engine::Strong<IntensityPositionTexture>& locked = texs.lock())
        {
            m_position_texture_ = locked;   
        }
    }

    void SetTargetLight(const UINT target_light)
    {
        m_target_light_ = target_light;
    }
    
protected:
    ShadowIntensityComputeShader( const std::filesystem::path &path );
    ShadowIntensityComputeShader();
    void preDispatch( const Engine::GraphicInterfaceContextPrimitive *context,
                      Engine::Graphics::SBs::LocalParamSB &           param,
                      const float                                     dt
            ) override;
    void postDispatch( const Engine::GraphicInterfaceContextPrimitive *context,
                       Engine::Graphics::SBs::LocalParamSB &           param,
                       const float                                     dt
            ) override;
    void                     loadDerived() override;
    void                     unloadDerived() override;

private:
    Engine::Weak<Engine::StructuredBufferTypeProxy<LightTableSB>> m_light_table_ptr_;
    Engine::Strong<IntensityTexture>            m_intersection_texture_;
    Engine::Strong<IntensityPositionTexture>    m_position_texture_;
    UINT                                        m_target_light_;
};
