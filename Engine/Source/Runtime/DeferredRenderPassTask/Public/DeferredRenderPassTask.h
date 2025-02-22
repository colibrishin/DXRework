#pragma once
#include <array>

#include "RenderTask.h"
#include "SingletonSpinLock/Public/SingletonSpinLock.h"

#include <ranges>

#include "DeferredRenderPassTask.generated.h"

namespace Engine
{
    struct TexturePair
    {
        TexturePair() = default;

        TexturePair( const std::array<Strong<Resources::Texture>, g_max_texture_per_material> *textures,
                     const std::array<Strong<Resources::Texture>, RESERVED_USER_TEX_END - RESERVED_USER_TEX_BEGIN>
                             *reservedTextures )
            : textures( textures ),
              reservedTextures( reservedTextures ),
              boundTextureCount( std::ranges::count_if(
                      *textures, []( const Strong<Resources::Texture> &tex ) { return tex != nullptr; } ))
        {
        }

        const std::array<Strong<Resources::Texture>, g_max_texture_per_material>                      *textures;
        const std::array<Strong<Resources::Texture>, RESERVED_USER_TEX_END - RESERVED_USER_TEX_BEGIN> *reservedTextures;

        size_t GetTextureCount() const
        {
            return boundTextureCount;
        }

    private:
        size_t boundTextureCount;
    };

    ECLASS(virtual)
    struct ENGINE_DEFERREDRENDERPASSTASK_API DeferredRenderPassTask : public RenderPassTask
    {
        GENERATE_BODY

        static constexpr size_t deferred_count = 4;
        static constexpr std::array<eFormat, deferred_count> deferred_format = {
            TEX_FORMAT_R32G32B32A32_FLOAT, TEX_FORMAT_R8G8B8A8_UNORM, TEX_FORMAT_R8G8B8A8_UNORM, TEX_FORMAT_R32G32B32A32_FLOAT
        };

    public:
        DeferredRenderPassTask();

        void Run( float                                                             dt,
                  bool                                                              shader_bypass,
                  RenderMap const                                                  *domain_map,
                  const aligned_vector<const StructuredBufferDecorator *>          &additional_sbs,
                  const Graphics::SBs::LocalParamSB                                &local_param,
                  const ObjectPredication                                          &predicate,
                  const ContextSetupFunction                                       &prerender_predicate,
                  const ContextSetupFunction                                       &postrender_predicate,
                  const std::unordered_map<std::string_view, ContextSetupFunction> &prerender_predicates,
                  const std::unordered_map<std::string_view, ContextSetupFunction> &postrender_predicates ) override;
        
        void Cleanup() override;
        void SetTexture( const Weak<Resources::Texture2D> &tex, const size_t slot );
        void SetDepthStencil( const Weak<Resources::Texture2D> &tex );
        void SetMaterialShader( const Weak<Resources::Shader> &shader );
        void SetLightShader( const Weak<Resources::Shader> &shader );

    private:
        using IntermediateShaderMap = concurrent_fast_pool_map<Resources::ShaderBase *, aligned_vector<InstancePair>>;

        inline void PredicateObject( const ObjectPredication &predicate,
                                     RenderMap const         *domain_map,
                                     uint64_t                &instance_count,
                                     IntermediateShaderMap   &out_map ) const;
        inline void
        StartPhase_MultiThread( float                                                             dt,
                                bool                                                              shader_bypass,
                                const Resources::Shader                                          *shader,
                                const Resources::Mesh                                            *mesh,
                                const aligned_vector<const StructuredBufferDecorator *>          &additional_sbs,
                                const Graphics::SBs::LocalParamSB                                &local_param,
                                const ContextSetupFunction                                       &prerender_predicate,
                                const ContextSetupFunction                                       &postrender_predicate,
                                const std::unordered_map<std::string_view, ContextSetupFunction> &prerender_predicates,
                                const std::unordered_map<std::string_view, ContextSetupFunction> &postrender_predicates,
                                const aligned_vector<InstancePair>                               &instance_pairs );

        inline void MaterialPass_Multithread( float                                                 dt,
                                           bool                                                  shader_bypass,
                                           size_t                                                instance_count,
                                           StructuredBufferTypeProxy<Graphics::SBs::InstanceSB> &instance_buffer,
                                           const Resources::Shader                              *shader,
                                           const Resources::Mesh                                *mesh,
                                           const GraphicInterfaceContextPrimitive               *context,
                                           const aligned_vector<Graphics::SBs::InstanceSB *>    &instances,
                                           const aligned_vector<TexturePair>                    &texture_pairs );

        inline void LightPass( 
                        const float                                                       dt,
                        const bool                                                        shader_bypass,
                        const aligned_vector<const StructuredBufferDecorator *>          &additional_sbs,
                        const Graphics::SBs::LocalParamSB                                &local_param,
                        const ContextSetupFunction                                       &prerender_predicate,
                        const ContextSetupFunction                                       &postrender_predicate,
                        const std::unordered_map<std::string_view, ContextSetupFunction> &prerender_predicates,
                        const std::unordered_map<std::string_view, ContextSetupFunction> &postrender_predicates );

        [[nodiscard]] void RecordUsedTexture( const GraphicInterfaceContextPrimitive *context,
                                              GraphicInterface                       &gi,
                                              const Resources::Texture               *tex );

    public:
        void PreRun( const RenderMap *render_map,
                const size_t render_map_count,
                const ObjectPredication &predication
                ) override;

    private:
        SpinLockTicket m_gi_ticket_;
        SpinLockTicket m_local_param_pool_ticket;
        SpinLockTicket m_instance_pool_ticket;
        SpinLockTicket m_texture_record_ticket_;

        StructuredBufferMemoryPool<Graphics::SBs::LocalParamSB> m_local_param_pool_{};
        StructuredBufferMemoryPool<Graphics::SBs::InstanceSB>   m_instance_pool_{};
        tbb::concurrent_vector<Unique<GraphicHeapBase>>         m_heaps_{};
        std::vector<const Resources::Texture *>                 m_used_shader_textures_{};

        Strong<Resources::Shader> m_material_pass_shader_;
        Strong<Resources::Shader> m_light_pass_shader_;

        Strong<Resources::Texture2D> m_deferred_render_targets_[ deferred_count ]{};
        Resources::Texture* m_deferred_render_targets_raw_[ deferred_count ]{};
        Strong<Resources::Texture2D> m_deferred_depth_{};
    };
}