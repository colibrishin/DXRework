#pragma once
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#include "Source/Runtime/Managers/Renderer/Public/RenderTask.h"

namespace Engine
{
	struct PIPELINERENDERPREREQUISITETASKDX12_API DX12PipelineRenderPassPrerequisiteTask : public PipelineRenderPrerequisiteTask
	{
		void Run(RenderPassTask* task_context) override;
		void Cleanup(RenderPassTask* task_context) override;
	};
}