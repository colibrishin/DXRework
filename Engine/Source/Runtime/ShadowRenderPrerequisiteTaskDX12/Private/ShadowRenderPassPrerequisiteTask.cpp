#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#include "Source/Runtime/RenderPassTaskDX12/Public/RenderPassTaskDX12.h"
#include "Source/Runtime/ShadowRenderPrerequisiteTaskDX12/Public/ShadowRenderPrerequisiteTaks.h"

namespace Engine
{
	void DX12ShadowRenderPassPrerequisiteTask::Run(RenderPassTask* task_context)
	{
		const DX12RenderPassTask* dx12_task = reinterpret_cast<DX12RenderPassTask*>(task_context);

		ID3D12GraphicsCommandList1* cmd = dx12_task->GetCurrentCommandList()->GetList4();

		if (IsLazy())
		{
			m_light_sb_.SetData(cmd, (UINT)m_lights_.size(), m_lights_.data());
			m_light_vp_sb_.SetData(cmd, (UINT)m_light_vps_.size(), m_light_vps_.data());

			FlipLazy();
		}

		Managers::RenderPipeline::GetInstance().SetPSO(dx12_task->GetCurrentCommandList(), m_shadow_shader_);

		m_light_sb_.CopySRVHeap(dx12_task->GetCurrentHeap());
		m_light_vp_sb_.CopySRVHeap(dx12_task->GetCurrentHeap());
		m_light_sb_.TransitionToSRV(cmd);
		m_light_vp_sb_.TransitionToSRV(cmd);
	}
	void DX12ShadowRenderPassPrerequisiteTask::Cleanup(RenderPassTask* task_context)
	{
		const DX12RenderPassTask* dx12_task = reinterpret_cast<DX12RenderPassTask*>(task_context);

		m_light_sb_.TransitionCommon(dx12_task->GetCurrentCommandList()->GetList4(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		m_light_vp_sb_.TransitionCommon(dx12_task->GetCurrentCommandList()->GetList4(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	}
}
