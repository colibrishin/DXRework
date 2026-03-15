#include "ReflectionEvaluatorModule.h"

#include "EngineEntryPoint.h"
#include "ReflectionEvaluator.h"
#include "Renderer.h"

MODULE_IMPL(Engine::ReflectionEvaluatorModule, ReflectionEvaluator)

bool Engine::ReflectionEvaluatorModule::InitializeImpl()
{
    CoreLoop::AddManager(
        CoreLoop::LOOP_TYPE_RENDER,
        &Managers::ReflectionEvaluator::GetInstance);
    
    Managers::Renderer::GetInstance().RegisterContextPreRenderSetup(
        "BindReflectionMap",
        [](const IGraphicContext* prim)
        {
            Managers::ReflectionEvaluator::GetInstance().BindReflectionMap(prim);
        });

    Managers::Renderer::GetInstance().RegisterContextPostRenderSetup(
        "UnbindReflectionMap",
        [](const IGraphicContext* prim)
        {
            Managers::ReflectionEvaluator::GetInstance().UnbindReflectionMap(prim);
        });

    return true;
}

bool Engine::ReflectionEvaluatorModule::ShutdownImpl()
{
    CoreLoop::RemoveManager(CoreLoop::LOOP_TYPE_RENDER, &Managers::ReflectionEvaluator::GetInstance);
    Managers::Renderer::GetInstance().UnregisterContextPreRenderSetup("BindReflectionMap");
    Managers::Renderer::GetInstance().UnregisterContextPostRenderSetup("UnbindReflectionMap");
    
    return true;
}

bool Engine::ReflectionEvaluatorModule::DynamicLoadable()
{
    return true;
}
