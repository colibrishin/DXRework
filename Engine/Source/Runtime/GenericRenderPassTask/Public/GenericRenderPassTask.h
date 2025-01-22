#pragma once
#include <memory>

#include "Source/Runtime/Core/ModuleManager/Public/IModule.h"

#include "Source/Runtime/Core/GraphicInterface.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderTask.h"
#include "Texture.h"

#include "GenericRenderPassTask.generated.h"

#include "SingletonSpinLock/Public/SingletonSpinLock.h"

namespace Engine
{
	struct TexturePair
	{
		TexturePair() = default;
		
		explicit TexturePair(
			const std::array<Strong<Resources::Texture>, g_max_texture_per_material>* textures,
			const std::array<Strong<Resources::Texture>, RESERVED_USER_TEX_END - RESERVED_USER_TEX_BEGIN>* reservedTextures)
			: textures(textures),
			  reservedTextures(reservedTextures) {}

		const std::array<Strong<Resources::Texture>, g_max_texture_per_material>* textures;
		const std::array<Strong<Resources::Texture>, RESERVED_USER_TEX_END - RESERVED_USER_TEX_BEGIN>* reservedTextures;
	};
	
	ECLASS()
	struct ENGINE_GENERICRENDERPASSTASK_API GenericRenderPassTask : RenderPassTask
	{
		GENERATE_BODY
		GenericRenderPassTask();
		GenericRenderPassTask& operator=(GenericRenderPassTask&) = delete;
		GenericRenderPassTask(GenericRenderPassTask&) = delete;

		void Run(
			float                              dt,
			bool                               shader_bypass,
			RenderMap const*                   domain_map,
			const Graphics::SBs::LocalParamSB& local_param,
			const ObjectPredication&           predicate,
			const ContextSetupFunction&		   prerender_predicate,
			const ContextSetupFunction&		   postrender_predicate
		) override;
		
		void Cleanup() override;

	private:
		using IntermediateMeshMap = concurrent_fast_pool_map<Weak<Resources::Mesh>, aligned_vector<const InstancePair*>>;
		
		inline IntermediateMeshMap PredicateObject(const ObjectPredication& predicate, RenderMap const* domain_map);

		inline void StartPhase_MultiThread(
			float dt,
			bool shader_bypass,
			const Weak<Resources::Mesh>& mesh,
			const Weak<Resources::Shader>& shader,
			const Graphics::SBs::LocalParamSB& local_param,
			const ContextSetupFunction& prerender_predicate,
			const ContextSetupFunction& postrender_predicate, const aligned_vector<const InstancePair*>& instance_pairs
		);


		inline void DrawPhase_MultiThread(
			float dt,
			bool shader_bypass,
			size_t instance_count,
			StructuredBufferTypeProxy<Graphics::SBs::InstanceSB>& instance_buffer,
			const Weak<Resources::Shader>& shader,
			const Weak<Resources::Mesh>& mesh, const GraphicInterfaceContextPrimitive* context, const aligned_vector<Graphics::
			SBs::InstanceSB*>& instances, const aligned_vector<TexturePair>& texture_pairs
		);

		SpinLockTicket m_gi_ticket_;
		SpinLockTicket m_local_param_pool_ticket;
		SpinLockTicket m_instance_pool_ticket;
		
		StructuredBufferMemoryPool<Graphics::SBs::LocalParamSB> m_local_param_pool_{};
		StructuredBufferMemoryPool<Graphics::SBs::InstanceSB> m_instance_pool_{};
		tbb::concurrent_vector<Unique<GraphicHeapBase>> m_heaps_{};
	};
}
