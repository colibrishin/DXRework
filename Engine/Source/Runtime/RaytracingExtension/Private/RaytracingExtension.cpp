#if CFG_RAYTRACING
#include "RaytracingExtension.h"

#include <functional>

#include "IGraphicAPI.h"
#include "IRaytracingExtension.h"
#include "RaytracingRenderPassTask.h"
#include "Renderer.h"

#if CFG_RENDERTYPE_DEFERRED
#include "DeferredRenderPassTask.h"
#endif
#include "ForwardRenderPassTask.h"

using RenderPassFn = std::function<void( std::wstring_view, Engine::eShaderDomain )>;

RenderPassFn RenderPass( const bool go )
{
    if ( go )
    {
        return []( std::wstring_view name, Engine::eShaderDomain domain )
        {
            Engine::Managers::Renderer::GetInstance().RenderPassWith( name, domain, {} );
        };
    }
    return []( std::wstring_view name, Engine::eShaderDomain domain )
    {
        Engine::Managers::Renderer::GetInstance().RenderPassWithout( name, domain );
    };
}

void Engine::RaytracingExtension::SetRaytracing( const bool flag )
{
    const std::string_view& rt_type_name = RaytracingRenderPassTask::StaticTypeName();
    const std::wstring rt_type_name_wstr(rt_type_name.begin(), rt_type_name.end());

#if CFG_RENDERTYPE_DEFERRED
    const std::string_view& df_type_name = DeferredRenderPassTask::StaticTypeName();
    const std::wstring df_type_name_wstr(df_type_name.begin(), df_type_name.end());
#endif

    const std::string_view& fd_type_name = ForwardRenderPassTask::StaticTypeName();
    const std::wstring fd_type_name_wstr(fd_type_name.begin(), fd_type_name.end());
    
    IRaytracingExtension& rgi = g_graphic_accessor.GetRaytracingInterface();
    rgi.UseRaytracing( flag );

    RenderPass(flag)( rt_type_name_wstr, SHADER_DOMAIN_OPAQUE );
#if CFG_RENDERTYPE_DEFERRED
    RenderPass(!flag)( df_type_name_wstr, SHADER_DOMAIN_OPAQUE );
#endif
#if CFG_RENDERTYPE_FORWARDONLY
    for (size_t i = 0; i < SHADER_DOMAIN_MAX; ++i)
    {
        RenderPass(!flag)( df_type_name_wstr, (eShaderDomain)i );
    }
#endif
}
#endif