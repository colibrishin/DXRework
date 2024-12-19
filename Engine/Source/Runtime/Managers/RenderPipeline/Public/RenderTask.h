#pragma once
#include <memory>
#include "RenderType.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

namespace Engine 
{
	struct RENDERPIPELINE_API RenderInstanceTask
	{
		virtual      ~RenderInstanceTask() = default;
		virtual void Run(Scene const* scene, const RenderMapValueType* render_map, std::atomic<uint64_t>& instance_count) = 0;
		virtual void Cleanup(RenderMapValueType* render_map) = 0;
	};

	struct RenderPassTask;

	struct RENDERPIPELINE_API RenderPassPrerequisiteTask
	{
		virtual      ~RenderPassPrerequisiteTask() = default;
		virtual void Run(RenderPassTask* task_context) = 0;
		virtual void Cleanup(RenderPassTask* task_context) = 0;
	};

	struct RENDERPIPELINE_API RenderPassTask
	{
		virtual      ~RenderPassTask() = default;
		virtual void Run(
			float                              dt,
			bool                               shader_bypass,
			RenderMap const*                   domain_map,
			const Graphics::SBs::LocalParamSB& local_param,
			const std::atomic<uint64_t>&       instance_count,
			RenderPassPrerequisiteTask* const* prerequisite,
			size_t                             prerequisite_count,
			const ObjectPredication&           predicate
		) = 0;

		virtual void Cleanup() = 0;
	};

	struct RENDERPIPELINE_API GenericRenderPassTask : RenderPassTask
	{
		void Run(
			float                              dt,
			bool                               shader_bypass,
			RenderMap const*                   domain_map,
			const Graphics::SBs::LocalParamSB& local_param,
			const std::atomic<uint64_t>&       instance_count,
			RenderPassPrerequisiteTask* const* prerequisite,
			size_t                             prerequisite_count,
			const ObjectPredication&           predicate
		) override;
		
		void Cleanup() override;

		[[nodiscard]] CommandListBase* GetCurrentCommandList() const;
		[[nodiscard]] GraphicHeapBase* GetCurrentHeap() const;

	private:
		void RunImpl(
			float                                                     dt,
			bool                                                      shader_bypass,
			IStructuredBufferType<Graphics::SBs::InstanceSB>& instance_buffer,
			const Weak<Resources::Material>&                          material,
			const GraphicInterfaceContextPrimitive*                   context,
			const aligned_vector<const Graphics::SBs::InstanceSB*>&   structuredbuffers
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
		aligned_vector<Unique<GraphicHeapBase>> m_heaps_;
	};
}