#include "ReflectionEvaluatorModule.h"

#include "ReflectionEvaluator.h"
#include "Renderer.h"

#include "CoreModuel/Public/CoreModule.h"

#include "ModuleManager/Public/ModuleManager.h"

MODULE_IMPL(Engine::ReflectionEvaluatorModule, ReflectionEvaluator)

void Engine::ReflectionEvaluatorModule::Initialize()
{
    CoreModule::GetContext().AddManager(
        CoreLoop::LOOP_TYPE_RENDER,
        &Managers::ReflectionEvaluator::GetInstance);
    
    Managers::Renderer::GetInstance().RegisterContextPreRenderSetup(
        "BindReflectionMap",
        [](const GraphicInterfaceContextPrimitive* prim)
        {
            Managers::ReflectionEvaluator::GetInstance().BindReflectionMap(prim);
        });

    Managers::Renderer::GetInstance().RegisterContextPostRenderSetup(
        "UnbindReflectionMap",
        [](const GraphicInterfaceContextPrimitive* prim)
        {
            Managers::ReflectionEvaluator::GetInstance().UnbindReflectionMap(prim);
        });
}

void Engine::ReflectionEvaluatorModule::Shutdown()
{
    CoreModule::GetContext().RemoveManager(CoreLoop::LOOP_TYPE_RENDER, &Managers::ReflectionEvaluator::GetInstance);
    Managers::Renderer::GetInstance().UnregisterContextPreRenderSetup("BindReflectionMap");
    Managers::Renderer::GetInstance().UnregisterContextPostRenderSetup("UnbindReflectionMap");
}

bool Engine::ReflectionEvaluatorModule::DynamicLoadable()
{
    return true;
}
