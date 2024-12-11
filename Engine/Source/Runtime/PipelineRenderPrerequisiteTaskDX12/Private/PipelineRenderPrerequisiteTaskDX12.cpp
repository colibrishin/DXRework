#include "../Public/PipelineRenderPrerequisiteTaskDX12.h"

#include "Source/Runtime/Resources/Shader/Public/Shader.hpp"

#include "source/runtime/renderpasstaskdx12/public/RenderPassTaskDX12.h"

namespace Engine
{
	void DX12PipelineRenderPassPrerequisiteTask::Run(RenderPassTask* task_context)
	{
		if (const PrimitivePipeline* pipeline = GetPrimitivePipeline())
		{
			const auto dx12_task = reinterpret_cast<DX12RenderPassTask*>(task_context);
			const CommandPair* cmd = dx12_task->GetCurrentCommandList();

			cmd->GetList()->SetGraphicsRootSignature(static_cast<ID3D12RootSignature*>(pipeline->GetNativePipeline()));
		}
	}

	void DX12PipelineRenderPassPrerequisiteTask::Cleanup(RenderPassTask* task_context)
	{
		
	}
}
