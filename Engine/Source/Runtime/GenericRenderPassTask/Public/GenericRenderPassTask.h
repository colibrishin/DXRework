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
		
		TexturePair(
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
			float                                                             dt,
			bool                                                              shader_bypass,
			RenderMap const*                                                  domain_map,
			const aligned_vector<const StructuredBufferDecorator*>&           additional_sbs,
			const Graphics::SBs::LocalParamSB&                                local_param,
			const ObjectPredication&                                          predicate,
			const ContextSetupFunction&                                       prerender_predicate,
			const ContextSetupFunction&                                       postrender_predicate,
			const std::unordered_map<std::string_view, ContextSetupFunction>& prerender_predicates,
			const std::unordered_map<std::string_view, ContextSetupFunction>& postrender_predicates
		) override;
		
		void Cleanup() override;

	private:
		using IntermediateMeshMap = concurrent_fast_pool_map<Weak<Resources::Mesh>, aligned_vector<InstancePair>>;
		
		inline void PredicateObject(const ObjectPredication& predicate, RenderMap const* domain_map, uint64_t& instance_count, IntermediateMeshMap& out_map) const;

		inline void StartPhase_MultiThread(
			float                                                             dt,
			bool                                                              shader_bypass,
			const Weak<Resources::Mesh>&                                      mesh,
			const Weak<Resources::Shader>&                                    shader,
			const aligned_vector<const StructuredBufferDecorator*>&           additional_sbs,
			const Graphics::SBs::LocalParamSB&                                local_param,
			const ContextSetupFunction&                                       prerender_predicate,
			const ContextSetupFunction&                                       postrender_predicate,
			const std::unordered_map<std::string_view, ContextSetupFunction>& prerender_predicates,
			const std::unordered_map<std::string_view, ContextSetupFunction>& postrender_predicates,
			const aligned_vector<InstancePair>&                               instance_pairs
		);
		[[nodiscard]] void RecordUsedTexture(
			const GraphicInterfaceContextPrimitive* context, GraphicInterface& gi, const Strong<Resources::Texture>& tex
		);


		inline void DrawPhase_MultiThread(
			float                                                 dt,
			bool                                                  shader_bypass,
			size_t                                                instance_count,
			StructuredBufferTypeProxy<Graphics::SBs::InstanceSB>& instance_buffer,
			const Weak<Resources::Shader>&                        shader,
			const Weak<Resources::Mesh>&                          mesh,
			const GraphicInterfaceContextPrimitive*               context,
			const aligned_vector<Graphics::SBs::InstanceSB*>&     instances,
			const aligned_vector<TexturePair>&                    texture_pairs
		);

		SpinLockTicket m_gi_ticket_;
		SpinLockTicket m_local_param_pool_ticket;
		SpinLockTicket m_instance_pool_ticket;
		SpinLockTicket m_texture_record_ticket_;

		StructuredBufferMemoryPool<Graphics::SBs::LocalParamSB> m_local_param_pool_{};
		StructuredBufferMemoryPool<Graphics::SBs::InstanceSB> m_instance_pool_{};
		tbb::concurrent_vector<Unique<GraphicHeapBase>> m_heaps_{};
		std::vector<Resources::Texture*> m_used_shader_textures_{};
	};
}
