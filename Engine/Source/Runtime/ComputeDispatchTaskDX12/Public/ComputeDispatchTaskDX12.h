#pragma once
#include "Source/Runtime/Resources/ComputeShader/Public/ComputeShader.h"

namespace Engine 
{
	struct COMPUTEDISPATCHTASKDX12_API DX12ComputeDispatchTask : public ComputeDispatchTask
	{
		DX12ComputeDispatchTask();

		void Dispatch(
			const Weak<Resources::ComputeShader>& w_shader, 
			const Graphics::SBs::LocalParamSB& local_param, 
			const UINT group_count[3],
			ComputeDispatchPrerequisiteTask* const* prerequisites, 
			const size_t prerequisite_count) override;
		void Cleanup() override;

		[[nodiscard]] CommandListBase* GetCommandPair() const;
		[[nodiscard]] GraphicHeapBase* GetHeap() const;

	private:
		CommandListBase* m_current_cmd_ = nullptr;
		GraphicHeapBase* m_current_heap_ = nullptr;
		
		aligned_vector<Unique<GraphicHeapBase>> m_heaps_{};
		StructuredBufferMemoryPool<Graphics::SBs::LocalParamSB> m_local_param_pool_;
	};
}