#include "ClientModule/Public/ClientModule.h"
#include "ClientModule.generated.h"
#include "Renderer.h"

#include "ModuleManager/Public/ModuleManager.h"
#include "RenderTasks/Public/ShadowIntersectionRenderTask.h"

MODULE_IMPL( ClientModule, Client )

bool ClientModule::InitializeImpl()
{
    Engine::Managers::Renderer::GetInstance().RegisterRenderPass( L"ShadowIntersectionRenderTask",
        new ShadowIntersectionRenderTask() );
    
    return IClientModule::InitializeImpl();
}

bool ClientModule::ShutdownImpl()
{
    Engine::Managers::Renderer::GetInstance().UnregisterRenderPass( L"ShadowIntersectionRenderTask" );
    return IClientModule::ShutdownImpl();
}

bool ClientModule::DynamicLoadable()
{
    return true;
}
