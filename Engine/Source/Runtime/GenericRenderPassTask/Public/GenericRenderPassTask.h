#pragma once
#include <memory>
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderTask.h"
#include "Source/Runtime/Core/GraphicInterface.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

namespace Engine 
{
	struct GENERICRENDERPASSTASK_API GenericRenderPassTask : RenderPassTask
	{
		void Run(
			float                              dt,
			bool                               shader_bypass,
			RenderMap const*                   domain_map,
			const Graphics::SBs::LocalParamSB& local_param,
			const std::atomic<uint64_t>&       instance_count,
			const ObjectPredication&           predicate,
			const ContextSetupFunction&		   prerender_predicate,
			const ContextSetupFunction&		   postrender_predicate
		) override;
		
		void Cleanup() override;

		[[nodiscard]] CommandListBase* GetCurrentCommandList() const;
		[[nodiscard]] GraphicHeapBase* GetCurrentHeap() const;

	private:
		void RunImpl(
			float                                                   dt,
			bool                                                    shader_bypass,
			IStructuredBufferType<Graphics::SBs::InstanceSB>&       instance_buffer,
			const Weak<Resources::Material>&                        material,
			const GraphicInterfaceContextPrimitive*                 context,
			const aligned_vector<const Graphics::SBs::InstanceSB*>& structuredbuffers
		);

		void DrawPhase(
			const bool shader_bypass,
			const uint64_t instance_count,
			const Weak<Resources::Material>& material,
			const GraphicInterfaceContextPrimitive* context);

		CommandListBase* m_current_cmd_ = nullptr;
		GraphicHeapBase* m_current_heap_ = nullptr;

		std::map<uint64_t, Unique<IStructuredBufferType<Graphics::SBs::MaterialSB>>> m_material_sbs_;
		std::set<uint64_t> m_updated_material_in_current_pass_;
		StructuredBufferMemoryPool<Graphics::SBs::LocalParamSB> m_local_param_pool_;
		StructuredBufferMemoryPool<Graphics::SBs::InstanceSB> m_instance_pool_;
		aligned_vector<Unique<GraphicHeapBase>> m_heaps_{};
	};
}