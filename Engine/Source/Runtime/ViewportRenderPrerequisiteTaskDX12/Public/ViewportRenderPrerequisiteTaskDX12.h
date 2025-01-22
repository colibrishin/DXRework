#pragma once

#include <directx/d3d12.h>

#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"

namespace Engine
{
	struct DX12ViewportRenderPrerequisiteTask : public ViewportRenderPrerequisiteTask
	{
		virtual void Run(RenderPassTask* task) override;
		virtual void Cleanup(RenderPassTask* task_context) override;

		[[nodiscard]] const D3D12_VIEWPORT& GetNativeViewport() const;
		[[nodiscard]] const D3D12_RECT& GetNativeScissorRect() const;

	private:
		D3D12_VIEWPORT m_d3d_viewport_{};
		D3D12_RECT m_d3d_scissor_{};
	};
}
