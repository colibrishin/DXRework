#pragma once
#include <memory>

#include "Source/Runtime/Core/ModuleManager/Public/IModule.h"

#include "Source/Runtime/Core/GraphicInterface.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderTask.h"

namespace Engine
{
	class GenericRenderPassTaskModule;
}

POLYMORPHIC_TYPE_MAP(Engine::GenericRenderPassTaskModule, Engine::IModule)

namespace Engine 
{
	struct ENGINE_GENERICRENDERPASSTASK_API GenericRenderPassTaskModule : public IModule
	{
		INLINE_COMPILE_TIME_TYPENAME(GenericRenderPassTaskModule)
		void Initialize() override;
		void Shutdown() override;
		bool DynamicLoadable() override;
	};
}

namespace Engine
{
	class GenericRenderPassTask;
}

POLYMORPHIC_TYPE_MAP(Engine::GenericRenderPassTask, Engine::RenderPassTask)

namespace Engine
{
	struct ENGINE_GENERICRENDERPASSTASK_API GenericRenderPassTask : RenderPassTask
	{
		INLINE_COMPILE_TIME_TYPENAME(GenericRenderPassTask)
		GenericRenderPassTask() = default;
		GenericRenderPassTask& operator=(GenericRenderPassTask&) = delete;
		GenericRenderPassTask(GenericRenderPassTask&) = delete;

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
			StructuredBufferTypeProxy<Graphics::SBs::InstanceSB>&       instance_buffer,
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

		std::map<uint64_t, StructuredBufferTypeProxy<Graphics::SBs::MaterialSB>> m_material_sbs_{};
		std::set<uint64_t> m_updated_material_in_current_pass_{};
		StructuredBufferMemoryPool<Graphics::SBs::LocalParamSB> m_local_param_pool_{};
		StructuredBufferMemoryPool<Graphics::SBs::InstanceSB> m_instance_pool_{};
		std::vector<Unique<GraphicHeapBase>> m_heaps_{};
	};
}