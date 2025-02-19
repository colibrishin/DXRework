#include "ForwardRenderPassTaskModule.h"
#include "ForwardRenderPassTaskModule.generated.h"
#include "Renderer.h"

#include "ForwardRenderPassTask.h"

MODULE_IMPL( Engine::ForwardRenderPassTaskModule, ForwardRenderPassTask )

namespace Engine
{
	bool ForwardRenderPassTaskModule::InitializeImpl()
	{
        std::string_view typename_str = ForwardRenderPassTask::StaticTypeName();
        std::wstring     typename_wstr( typename_str.begin(), typename_str.end() );

        Managers::Renderer::GetInstance().RegisterRenderPass( typename_wstr,
                                                              new ForwardRenderPassTask() );

#ifdef CFG_RENDERTYPE_FORWARDONLY
        Managers::Renderer::GetInstance().AddToMainPassTask( typename_wstr.data() );
#endif
		return true;
	}

	bool ForwardRenderPassTaskModule::ShutdownImpl()
	{
        std::string_view typename_str = ForwardRenderPassTask::StaticTypeName();
        std::wstring     typename_wstr( typename_str.begin(), typename_str.end() );

        Managers::Renderer::GetInstance().UnregisterRenderPass( typename_wstr );
		return true;
	}

	bool ForwardRenderPassTaskModule::DynamicLoadable()
	{
		return true;
	}
}
