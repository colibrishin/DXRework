#pragma once
#if CLIENT || WITH_EDITOR
#include "RenderPassTask.h"

#include "ShadowIntersectionRenderTask.generated.h"

class ShadowIntersectionComponent;

namespace Engine::Graphics::SBs
{
    struct LightVPSB;
}

ECLASS(virtual)
struct ENGINE_CLIENT_API ShadowIntersectionRenderTask : Engine::RenderPassTask
{
    GENERATE_BODY
    
    ShadowIntersectionRenderTask();
    ShadowIntersectionRenderTask& operator=(ShadowIntersectionRenderTask&) = delete;
    ShadowIntersectionRenderTask(ShadowIntersectionRenderTask&) = delete;

    ShadowIntersectionRenderTask& operator=(ShadowIntersectionRenderTask&& other) noexcept
    {
        m_tmp_shadow_depth_      = std::move( other.m_tmp_shadow_depth_ );
        m_intersection_compute_  = std::move( other.m_intersection_compute_ );
        m_shadow_shader_         = std::move( other.m_shadow_shader_ );
        m_intensity_test_shader_ = std::move( other.m_intensity_test_shader_ );

        return *this;
    }

    ShadowIntersectionRenderTask( ShadowIntersectionRenderTask&& other ) noexcept
    {
        m_tmp_shadow_depth_      = std::move( other.m_tmp_shadow_depth_ );
        m_intersection_compute_  = std::move( other.m_intersection_compute_ );
        m_shadow_shader_         = std::move( other.m_shadow_shader_ );
        m_intensity_test_shader_ = std::move( other.m_intensity_test_shader_ );
    }
    
    void Run( float dt,
            bool shader_bypass,
            const Engine::RenderMap *domain_map,
            const Engine::aligned_vector<const Engine::StructuredBufferDecorator *> &additional_sbs,
            const Engine::Graphics::SBs::LocalParamSB &local_param,
            const Engine::ObjectPredication &predicate,
            const Engine::ContextSetupFunction &prerender_predicate,
            const Engine::ContextSetupFunction &postrender_predicate,
            const std::unordered_map<std::string_view, Engine::ContextSetupFunction> &prerender_predicates,
            const std::unordered_map<std::string_view, Engine::ContextSetupFunction> &postrender_predicates
            ) override;
    void Cleanup() override;

private:
    void FirstPass(
            float dt,
            const Engine::Abstracts::ObjectBase* pivot,
            const ShadowIntersectionComponent* component,
            size_t shadow_slot,
            const Engine::Strong<Engine::Layer>& lights) const;

    void SecondPass(
            float dt,
            const ShadowIntersectionComponent* component,
            const std::vector<Engine::Graphics::SBs::LightVPSB>& light_vps,
            const Engine::Strong<Engine::Scene>& scene,
            const Engine::Strong<Engine::Layer>& lights) const;
    
    void ThirdPass(
        float dt,
        const ShadowIntersectionComponent* component,
        const Engine::Strong<Engine::Layer>& lights) const;

public:
    void PreRun( const Engine::RenderMap *render_map,
            const size_t render_map_count,
            const Engine::ObjectPredication &predication
            ) override;

private:
    Engine::Viewport m_viewport_ = {
        .topLeftX = 0,
        .topLeftY = 0,
        .width = CFG_CASCADE_SHADOW_TEX_WIDTH,
        .height = CFG_CASCADE_SHADOW_TEX_HEIGHT,
        .minDepth = 0.f,
        .maxDepth = 1.f 
    };

    Engine::Strong<Engine::Resources::Texture2D>     m_tmp_shadow_depth_;
    Engine::Strong<Engine::Resources::ComputeShader> m_intersection_compute_;
    Engine::Strong<Engine::Resources::Shader>        m_shadow_shader_;
    Engine::Strong<Engine::Resources::Shader>        m_intensity_test_shader_;
};
#endif