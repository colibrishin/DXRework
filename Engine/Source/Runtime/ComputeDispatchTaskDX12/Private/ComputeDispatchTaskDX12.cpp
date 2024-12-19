#include "ComputeDispatchTaskDX12.h"
#include "Source/Runtime/Managers/D3D12Wrapper/Public/D3Device.hpp"
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#include "Source/Runtime/PrimitivePipelineDX12/Public/PrimitivePipelineDX12.h"

Engine::DX12ComputeDispatchTask::DX12ComputeDispatchTask()
{
	m_local_param_pool_.resize(static_cast<size_t>(1) << 3);
}

void Engine::DX12ComputeDispatchTask::Dispatch(
	const Weak<Resources::ComputeShader>& w_shader, 
	const Graphics::SBs::LocalParamSB& local_param, 
	const UINT group_count[3],
	ComputeDispatchPrerequisiteTask* const* prerequisites, 
	const size_t prerequisite_count)
{
	if (const Strong<Resources::ComputeShader>& shader = w_shader.lock()) 
	{
		m_current_cmd_ = Managers::D3Device::GetInstance().AcquireCommandPair(D3D12_COMMAND_LIST_TYPE_DIRECT, L"Dispatch").lock().get();
		m_current_cmd_->SoftReset();

		auto pipeline = reinterpret_cast<DX12PrimitivePipeline*>(Managers::RenderPipeline::GetInstance().GetPrimitivePipeline());
		DescriptorHandler& handler = pipeline->GetHeapHandler();
		m_heaps_.push_back(handler.Acquire().lock());
		m_current_heap_ = m_heaps_.back().lock().get();

		m_current_cmd_->GetList()->SetComputeRootSignature(
			reinterpret_cast<ID3D12RootSignature*>(Managers::RenderPipeline::GetInstance().GetPrimitivePipeline()->GetNativePipeline()));

		Graphics::StructuredBuffer<Graphics::SBs::LocalParamSB>& sb = m_local_param_pool_.get();
		sb.SetData(m_current_cmd_->GetList(), 1, &local_param);
		sb.TransitionToSRV(m_current_cmd_->GetList());
		sb.CopySRVHeap(m_current_heap_);

		m_current_heap_->BindCompute(m_current_cmd_->GetList());

		m_current_cmd_->GetList()->SetPipelineState(
			reinterpret_cast<ID3D12PipelineState*>(shader->GetPrimitiveShader()->GetComputePrimitiveShader()));

		for (size_t i = 0; i < prerequisite_count; ++i)
		{
			prerequisites[i]->PreDispatch(this);
		}

		m_current_cmd_->GetList()->Dispatch(group_count[0], group_count[1], group_count[2]);

		sb.TransitionCommon(m_current_cmd_->GetList(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		
		m_current_cmd_->FlagReady();

		if (const Strong<CommandPair>& cleanup_cmd = Managers::D3Device::GetInstance().AcquireCommandPair(D3D12_COMMAND_LIST_TYPE_DIRECT, L"Dispatch Cleanup").lock())
		{
			cleanup_cmd->SoftReset();

			m_current_cmd_ = cleanup_cmd.get();

			for (size_t i = 0; i < prerequisite_count; ++i)
			{
				prerequisites[i]->PostDispatch(this);
			}

			cleanup_cmd->FlagReady();
			Managers::D3Device::GetInstance().WaitForCommandsCompletion();
		}

		m_local_param_pool_.advance();
	}
}

void Engine::DX12ComputeDispatchTask::Cleanup()
{
	m_local_param_pool_.reset();
	m_heaps_.clear();
}

Engine::CommandPair* Engine::DX12ComputeDispatchTask::GetCommandPair() const
{
	return m_current_cmd_;
}

Engine::DescriptorPtrImpl* Engine::DX12ComputeDispatchTask::GetHeap() const
{
	return m_current_heap_;
}
