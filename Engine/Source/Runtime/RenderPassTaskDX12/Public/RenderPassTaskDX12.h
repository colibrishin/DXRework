#pragma once
#include <map>
#include <set>

#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"

#include "Source/Runtime/Managers/D3D12Wrapper/Public/StructuredBufferDX12.hpp"
#include "Source/Runtime/Managers/D3D12Wrapper/Public/StructuredBufferMemoryPoolDX12.hpp"
#include "Source/Runtime/Resources/Material/Public/Material.h"
#include "Source/Runtime/CommandPair/Public/CommandPair.h"
#include "Source/Runtime/DescriptorHeap/Public/Descriptors.h"

namespace Engine
{
	struct DX12RenderPassTask : public RenderPassTask
	{
		void Run(
			const float dt,
			const bool shader_bypass,
			const eShaderDomain domain, 
			RenderMap const* domain_map,
			const Graphics::SBs::LocalParamSB& local_param,
			const std::atomic<uint64_t>& instance_count,
			RenderPassPrerequisiteTask* const* prerequisite, 
			const size_t prerequisite_count,
			const ObjectPredication& predicate) override;
		void Cleanup() override;

		CommandPair* GetCurrentCommandList() const;
		DescriptorPtrImpl* GetCurrentHeap() const;

	private:
		void RunImpl(
			const float dt,
			const bool shader_bypass,
			const eShaderDomain shader_domain,
			Graphics::StructuredBuffer<Graphics::SBs::InstanceSB>& instance_buffer,
			const Weak<Resources::Material>& material,
			const Weak<CommandPair>& command_pair,
			const DescriptorPtr& descriptor_ptr,
			const aligned_vector<const Graphics::SBs::InstanceSB*>& structurebuffers);

		void DrawPhase(
			const bool shader_bypass,
			const uint64_t instance_count,
			const Weak<Resources::Material>& material,
			const Weak<CommandPair>& w_cmd,
			const DescriptorPtr& w_heap);

		CommandListBase* m_current_cmd_;
		GraphicHeapBase* m_current_heap_;

		std::map<uint64_t, Graphics::StructuredBuffer<Graphics::SBs::MaterialSB>> m_material_sbs_;
		std::set<uint64_t> m_updated_material_in_current_pass_;
		Strong<StructuredBufferMemoryPoolBase<Graphics::SBs::LocalParamSB>> m_local_param_pool_;
		Strong<StructuredBufferMemoryPoolBase<Graphics::SBs::InstanceSB>> m_instance_pool_;
		aligned_vector<Unique<GraphicHeapBase>> m_heaps_;
	};
}