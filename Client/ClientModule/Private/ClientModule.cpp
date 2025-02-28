#include "ClientModule/Public/ClientModule.h"
#include "ClientModule.generated.h"

#if CLIENT || WITH_EDITOR
#include "Renderer.h"
#include "RenderTasks/Public/ShadowIntersectionRenderTask.h"
#endif

#if CLIENT
#include "INetworkAPI.h"
#endif

MODULE_IMPL( ClientModule, Client )

bool ClientModule::InitializeImpl()
{
#if CLIENT || WITH_EDITOR
    Engine::Managers::Renderer::GetInstance().RegisterRenderPass( L"ShadowIntersectionRenderTask",
        new Engine::RenderPassTaskFactory<ShadowIntersectionRenderTask>() );
#endif

#if CLIENT
    Engine::g_network_accessor.GetMessageTask().AddNewHost( {
            .ip = {192, 168, 0, 32},
              .tcp = 60902,
              .udp = 60901,
    } );
#endif

    return IClientModule::InitializeImpl();
}

bool ClientModule::ShutdownImpl()
{
#if CLIENT || WITH_EDITOR
    Engine::Managers::Renderer::GetInstance().UnregisterRenderPass( L"ShadowIntersectionRenderTask" );
#endif
    return IClientModule::ShutdownImpl();
}

bool ClientModule::DynamicLoadable()
{
    return true;
}
