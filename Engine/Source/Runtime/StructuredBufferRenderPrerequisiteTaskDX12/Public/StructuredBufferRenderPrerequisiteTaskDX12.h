#pragma once
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"

#include "Source/Runtime/RenderPassTaskDX12/Public/RenderPassTaskDX12.h"

namespace Engine
{
	template <typename T>
	struct DX12StructuredBufferRenderPrerequisiteTask : public StructuredBufferRenderPrerequisiteTask<T>
	{
		void Run(RenderPassTask* task_context) override;
		void Cleanup(RenderPassTask* task_context) override;

	private:
		Graphics::StructuredBuffer<T> m_buffer_;

	};

	template <typename T>
	void DX12StructuredBufferRenderPrerequisiteTask<T>::Run(RenderPassTask* task_context)
	{
		const auto dx12_task = reinterpret_cast<DX12RenderPassTask*>(task_context);

		if (this->IsDirty())
		{
			T* ptr = nullptr;
			size_t count = 0;
			this->GetData(&ptr, count);

			m_buffer_.SetData(dx12_task->GetCurrentCommandList(), count, ptr);
			this->FlipDirty();
		}

		m_buffer_.CopySRVHeap(dx12_task->GetCurrentHeap());
		m_buffer_.TransitionToSRV(dx12_task->GetCurrentCommandList());
	}

	template <typename T>
	void DX12StructuredBufferRenderPrerequisiteTask<T>::Cleanup(RenderPassTask* task_context)
	{
		const auto dx12_task = reinterpret_cast<DX12RenderPassTask*>(task_context);
		m_buffer_.TransitionCommon(dx12_task->GetCurrentCommandList(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	}
}
