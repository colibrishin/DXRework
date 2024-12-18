#include "../Public/ViewportRenderPrerequisiteTaskDX12.h"

#include "Source/Runtime/RenderPassTaskDX12/public/RenderPassTaskDX12.h"

namespace Engine
{
	void DX12ViewportRenderPrerequisiteTask::Run(RenderPassTask* task)
	{
		const Viewport viewport = GetViewport();
		m_d3d_viewport_ = 
			{
				.TopLeftX = viewport.topLeftX,
				.TopLeftY = viewport.topLeftY,
				.Width = viewport.width,
				.Height = viewport.height,
				.MinDepth = viewport.minDepth,
				.MaxDepth = viewport.maxDepth
			};

		m_d3d_scissor_ =
			{
			.left = 0,
			.top = 0,
			.right = static_cast<UINT>(viewport.width),
			.bottom = static_cast<UINT>(viewport.height) 
			};

		const DX12RenderPassTask* dx12_task = reinterpret_cast<DX12RenderPassTask*>(task);
		ID3D12GraphicsCommandList1* cmd = dx12_task->GetCurrentCommandList()->GetList();

		cmd->RSSetViewports(1, &m_d3d_viewport_);
		cmd->RSSetScissorRects(1, &m_d3d_scissor_);
	}

	void DX12ViewportRenderPrerequisiteTask::Cleanup(RenderPassTask* task_context)
	{
	}

	const D3D12_VIEWPORT& DX12ViewportRenderPrerequisiteTask::GetNativeViewport() const 
	{
		return m_d3d_viewport_;
	}

	const D3D12_RECT& DX12ViewportRenderPrerequisiteTask::GetNativeScissorRect() const 
	{
		return m_d3d_scissor_;
	}
}