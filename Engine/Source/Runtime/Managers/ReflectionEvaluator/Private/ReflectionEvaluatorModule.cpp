#include "ReflectionEvaluatorModule.h"

#include "EngineEntryPoint.h"
#include "ReflectionEvaluatorModule.generated.h"
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

const std::vector<std::string> & Engine::ReflectionEvaluatorModule::LoadAfter() const
{
    static const std::vector<std::string> load_after = { "RenderPipeline" };
    return load_after;
}
