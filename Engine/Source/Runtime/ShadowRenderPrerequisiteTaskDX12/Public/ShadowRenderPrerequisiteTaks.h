#pragma once
#include "Source/Runtime/Managers/Renderer/Public/ShadowManager.hpp"
#include "Source/Runtime/Managers/Renderer/Public/RenderTask.h"

namespace Engine
{
	struct SHADOWRENDERPREREQUISITETASK_API DX12ShadowRenderPassPrerequisiteTask : public ShadowRenderPrerequisiteTask
	{
		void Run(RenderPassTask* task_context) override;
		void Cleanup(RenderPassTask* task_context) override;

	private:
		Graphics::StructuredBuffer<Graphics::SBs::LightSB> m_light_sb_;
		Graphics::StructuredBuffer<Graphics::SBs::LightVPSB> m_light_vp_sb_;
	};
}