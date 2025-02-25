#pragma once
#include <memory>

#include "RenderTask.h"
#include "GraphicInterface.h"
#include "ConcurrentTypeLibrary.h"
#include "TypeLibrary.h"
#include "Texture.h"

#include "SingletonSpinLock.h"

#include "RaytracingRenderPassTask.generated.h"

namespace Engine
{
    struct RaytracingMeshInstancePair
    {
        Resources::Mesh* mesh;
        size_t instanceCount;
    };
    
	ECLASS()
	struct ENGINE_RAYTRACINGRENDERPASSTASK_API RaytracingRenderPassTask : RenderPassTask
	{
		GENERATE_BODY
		RaytracingRenderPassTask();
		RaytracingRenderPassTask& operator=(RaytracingRenderPassTask&) = delete;
		RaytracingRenderPassTask(RaytracingRenderPassTask&) = delete;

	    void PreRun(RenderMap const* render_map, const size_t render_map_count, const ObjectPredication& predication) override;
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
		using IntermediateShaderMap = concurrent_fast_pool_map<Resources::ShaderBase*, aligned_vector<InstancePair>>;
		
		inline void PredicateObject(const ObjectPredication& predicate, RenderMap const* domain_map, uint64_t& instance_count, IntermediateShaderMap& out_map) const;

        inline void StartPhase_MultiThread(
            float dt, bool shader_bypass, const std::vector<RaytracingMeshInstancePair>& meshes,
            const Resources::RaytracingShader* shader,
            const aligned_vector<const StructuredBufferDecorator*>& additional_sbs,
            const Graphics::SBs::LocalParamSB& local_param, const ContextSetupFunction& prerender_predicate,
            const ContextSetupFunction& postrender_predicate,
            const std::unordered_map<std::string_view, ContextSetupFunction>& prerender_predicates,
            const std::unordered_map<std::string_view, ContextSetupFunction>& postrender_predicates,
            const aligned_vector<InstancePair>& instance_pairs
        );
	    
		void RecordUsedTexture(
			const GraphicInterfaceContextPrimitive* context, GraphicInterface& gi, const Strong<Resources::Texture>& tex);

        byte_stream& GetByteStream();

        inline void DispatchPhase_MultiThread(
            float dt, bool shader_bypass, size_t instance_count,
            StructuredBufferTypeProxy<Graphics::SBs::InstanceSB>& instance_buffer,
            const Resources::RaytracingShader* shader, const byte_stream& hit_records,
            const GraphicInterfaceContextPrimitive* context, const aligned_vector<Graphics::SBs::InstanceSB*>& instances
        );

		SpinLockTicket m_gi_ticket_;
		SpinLockTicket m_local_param_pool_ticket;
		SpinLockTicket m_instance_pool_ticket;
		SpinLockTicket m_texture_record_ticket_;
	    SpinLockTicket m_heap_ticket_;
	    SpinLockTicket m_byte_stream_ticket_;

	    AccelStructBuffer m_top_level_acceleration_buffer_{};
	    std::vector<Unique<GraphicHeapBase>> m_local_heaps_{};

	    std::vector<byte_stream> m_byte_stream_{};
	    std::vector<uint32_t> m_byte_stream_usage_{};
	    
		StructuredBufferMemoryPool<Graphics::SBs::LocalParamSB> m_local_param_pool_{};
		StructuredBufferMemoryPool<Graphics::SBs::InstanceSB> m_instance_pool_{};
		std::vector<Resources::Texture*> m_used_shader_textures_{};
	};
}
