#pragma once
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#include "Source/Runtime/Managers/Renderer/Public/RenderTask.h"

namespace Engine
{
	struct SHADERRENDERPREREQUISITETASKDX12_API DX12ShaderRenderPassPrerequisiteTask : public ShaderRenderPrerequisiteTask
	{
		void Run(RenderPassTask* task_context) override;
		void Cleanup(RenderPassTask* task_context) override;
	};
}