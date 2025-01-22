#include "../Public/RenderTask.h"

namespace Engine
{
	void ViewportRenderPrerequisiteTask::SetViewport(const Viewport& viewport)
	{
		m_viewport_ = viewport;
	}

	Viewport ViewportRenderPrerequisiteTask::GetViewport() const
	{
		return m_viewport_;
	}

	void PipelineRenderPrerequisiteTask::SetPrimitivePipeline(PrimitivePipeline* pipeline)
	{
		if (pipeline)
		{
			m_pipeline_ = pipeline;
		}
	}

	PrimitivePipeline* PipelineRenderPrerequisiteTask::GetPrimitivePipeline() const
	{
		return m_pipeline_;
	}
}
