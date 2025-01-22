#pragma once
#include "Source/Runtime/CommandPair/Public/CommandPair.h"
#include "Source/Runtime/DescriptorHeap/Public/Descriptors.h"
#include "Source/Runtime/Resources/ComputeShader/Public/ComputeShader.h"
#include "Source/Runtime/Managers/D3D12Wrapper/Public/StructuredBufferMemoryPoolDX12.hpp"

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

		[[nodiscard]] CommandPair* GetCommandPair() const;
		[[nodiscard]] DescriptorPtrImpl* GetHeap() const;

	private:
		CommandPair* m_current_cmd_ = nullptr;
		DescriptorPtrImpl* m_current_heap_ = nullptr;
		
		aligned_vector<DescriptorPtr> m_heaps_{};
		Graphics::StructuredBufferMemoryPool<Graphics::SBs::LocalParamSB> m_local_param_pool_;
	};
}