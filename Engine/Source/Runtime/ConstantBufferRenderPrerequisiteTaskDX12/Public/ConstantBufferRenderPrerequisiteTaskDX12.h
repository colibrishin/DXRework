#pragma once
#include "Source/Runtime/Managers/D3D12Wrapper/Public/ConstantBufferDX12.hpp"

#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"

#include "Source/Runtime/RenderPassTaskDX12/Public/RenderPassTaskDX12.h"

namespace Engine
{
	template <typename T>
	struct DX12ConstantBufferRenderPrerequisiteTask : public ConstantBufferRenderPrerequisiteTask<T>
	{
		void Run(RenderPassTask* task_context) override;
		void Cleanup(RenderPassTask* task_context) override;

	private:
		Graphics::ConstantBuffer<T> m_buffer_;

	};

	template <typename T>
	void DX12ConstantBufferRenderPrerequisiteTask<T>::Run(RenderPassTask* task_context)
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

		m_buffer_.Bind(dx12_task->GetCurrentCommandList(), dx12_task->GetCurrentHeap());
	}

	template <typename T>
	void DX12ConstantBufferRenderPrerequisiteTask<T>::Cleanup(RenderPassTask* task_context)
	{
	}
}
