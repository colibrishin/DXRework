#include "ComputeShaders/Public/ShadowIntensityComputeShader.h"
#include "ShadowIntensityComputeShader.generated.h"

#include "Resources/Public/IntensityPositionTexture.h"
#include "Resources/Public/IntensityTexture.h"

using namespace Engine;

ShadowIntensityComputeShader::ShadowIntensityComputeShader( const std::filesystem::path   &path )
    : ComputeShader( path ),
      m_target_light_(0)
{
    SetThread( { 32, 32, 1 } );
}

ShadowIntensityComputeShader::ShadowIntensityComputeShader() : ComputeShader( "" ),
      m_target_light_(0)
{
    SetThread( { 32, 32, 1 } );
}

void ShadowIntensityComputeShader::preDispatch( const GraphicInterfaceContextPrimitive *context,
                                                Graphics::SBs::LocalParamSB &param,
                                                const float dt)
{
    if (m_light_table_ptr_.expired())
    {
        return;
    }

    const auto& table = m_light_table_ptr_.lock();

    table->CopyUAVHeap(context);
    table->TransitionToUAV(context);

    GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
    gi.Bind( context, m_intersection_texture_.get(), BIND_TYPE_SRV, BIND_SLOT_TEX, 0 );
    gi.Bind( context, m_position_texture_.get(), BIND_TYPE_SRV, BIND_SLOT_TEX, 1 );
    param.SetParam(target_light_slot, static_cast<int>(m_target_light_));
}

void ShadowIntensityComputeShader::postDispatch( const GraphicInterfaceContextPrimitive *context,
        Graphics::SBs::LocalParamSB &param,
        const float dt)
{
    if (m_light_table_ptr_.expired())
    {
        return;
    }

    const auto& table = m_light_table_ptr_.lock();
    table->TransitionCommon(context);

    GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
    gi.TransitBack( context, m_intersection_texture_.get(), BIND_TYPE_SRV );
    gi.TransitBack( context, m_position_texture_.get(), BIND_TYPE_SRV );

    m_light_table_ptr_      = {};
    m_target_light_         = 0;
    m_intersection_texture_ = nullptr;
}

void ShadowIntensityComputeShader::loadDerived()
{}

void ShadowIntensityComputeShader::unloadDerived()
{}