#include "../Public/ShaderRenderPrerequisiteTaskDX12.h"

#include "Source/Runtime/Resources/Shader/Public/Shader.hpp"

#include "source/runtime/graphicprimitiveshaderdx12/public/GraphicPrimitiveShaderDX12.h"
#include "source/runtime/renderpasstaskdx12/public/RenderPassTaskDX12.h"

namespace Engine
{
	void DX12ShaderRenderPassPrerequisiteTask::Run(RenderPassTask* task_context)
	{
		if (const auto shader = reinterpret_cast<const DX12GraphicPrimitiveShader*>(GetShader()))
		{
			const auto dx12_task = reinterpret_cast<DX12RenderPassTask*>(task_context);
			const CommandPair* cmd = dx12_task->GetCurrentCommandList();
			const DescriptorPtrImpl* heap = dx12_task->GetCurrentHeap();

			cmd->GetList()->SetPipelineState(static_cast<ID3D12PipelineState*>(shader->GetGraphicPrimitiveShader()));
			heap->SetSampler(shader->GetSamplerHeap()->GetCPUDescriptorHandleForHeapStart(), SAMPLER_TEXTURE);
		}
	}

	void DX12ShaderRenderPassPrerequisiteTask::Cleanup(RenderPassTask* task_context)
	{
	}
}
