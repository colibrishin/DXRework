#include "../Public/RenderType.h"
#include "../Public/RenderTask.h"

namespace Engine
{
	void* PrimitivePipeline::GetNativePipeline() const
	{
		return m_pipeline_;
	}

	void PrimitivePipeline::SetPrimitivePipeline(void* pipeline)
	{
		if (pipeline)
		{
			m_pipeline_ = pipeline;	
		}
	}
}
