#if CLIENT || WITH_EDITOR
#include "RenderTasks/Public/ShadowIntersectionRenderTask.h"

#include "RenderPipeline.h"
#include "ShadowIntersectionRenderTask.generated.h"


#include "Renderer.h"
#include "Shader.h"
#include "ShadowManager.h"

#include "Layer.h"
#include "ObjectBase.h"
#include "Scene.h"
#include "SceneManager.h"
#include "Components/Public/ShadowIntersectionComponent.h"
#include "Camera.h"
#include "Resources/Public/ShadowMaskTexture.h"
#include "ShadowTexture.h"
#include "Transform.h"

#include "Resources/Public/IntensityTexture.h"
#include "Resources/Public/IntensityPositionTexture.h"
#include "ForwardRenderPassTask.h"

using namespace Engine;

ShadowIntersectionRenderTask::ShadowIntersectionRenderTask()
{
    m_intensity_test_shader_ = Resources::Shader::Get("intensity_test").lock();
    m_shadow_shader_ = Resources::Shader::Get("cascade_shadow_stage1").lock();

    if ( const auto &cs = ShadowIntensityComputeShader::Get( "IntersectionCompute" ).lock() )
    {
        m_intersection_compute_ = cs;
    }
    else
    {
        const auto &new_cs = ShadowIntensityComputeShader::Create( "IntersectionCompute", "cs_intensity_test.hlsl");

        m_intersection_compute_ = new_cs;
    }

    m_tmp_shadow_depth_ = Resources::Texture2D::Create
            (
             "tmp_shadow_depth",
             "",
             GenericTextureDescription {
                 .Dimension = TEX_TYPE_2D,
                 .Alignment = 0,
                 .Width = CFG_CASCADE_SHADOW_TEX_WIDTH,
                 .Height = CFG_CASCADE_SHADOW_TEX_HEIGHT,
                 .DepthOrArraySize = 1,
                 .Format = TEX_FORMAT_D32_FLOAT,
                 .Flags = RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
                 .MipsLevel = 1,
                 .Layout = TEX_LAYOUT_UNKNOWN,
                 .SampleDesc = {1, 0},
                 .AsSRV = true,
                 .AsDSV = true,
                 .Srv = {
                     .Format = TEX_FORMAT_R32_FLOAT,
                     .ViewDimension = SRV_DIMENSION_TEXTURE2D,
                     .Shader4ComponentMapping = d3d12_shader4_component_mapping,
                     .Texture2D = {0, 1, 0, 0} 
                 }
             }
            );
}

void ShadowIntersectionRenderTask::Run( float dt,
                                        bool shader_bypass,
                                        const RenderMap *domain_map,
                                        const aligned_vector<const StructuredBufferDecorator *> &additional_sbs,
                                        const Graphics::SBs::LocalParamSB &local_param,
                                        const ObjectPredication &predicate,
                                        const ContextSetupFunction &prerender_predicate,
                                        const ContextSetupFunction &postrender_predicate,
                                        const std::unordered_map<std::string_view, ContextSetupFunction> &prerender_predicates,
                                        const std::unordered_map<std::string_view, ContextSetupFunction> &postrender_predicates
        )
{
    if ( const Strong<Scene>& scene = Managers::SceneManager::GetInstance().GetActiveScene().lock() )
    {
        for ( const auto& comps = scene->GetCachedComponents<ShadowIntersectionComponent>();
            const Weak<Abstracts::Component>& comp : comps)
        {
            if (const Strong<ShadowIntersectionComponent>& locked = Cast<ShadowIntersectionComponent>(comp))
            {
                const Strong<Abstracts::ObjectBase>& pivot = locked->GetOwner().lock();
                constexpr size_t                      shadow_slot = 0;
                
                FirstPass( dt, pivot.get(), locked.get(), shadow_slot, scene->at( RESERVED_LAYER_LIGHT ));
                SecondPass( dt,
                          locked.get(),
                          Managers::ShadowManager::GetInstance().GetCurrentSceneLightVP(),
                          scene,
                          scene->at( RESERVED_LAYER_LIGHT ) );
                ThirdPass( dt, locked.get(), scene->at( RESERVED_LAYER_LIGHT ) );
            }
        }
    }
}

void ShadowIntersectionRenderTask::Cleanup()
{
    
}

void ShadowIntersectionRenderTask::FirstPass(float dt,
    const Abstracts::ObjectBase*                   pivot,
    const ShadowIntersectionComponent*             component,
    const size_t                                   shadow_slot,
    const Strong<Layer> &                          lights) const
{
    IGraphicAPI& gi = g_graphic_accessor.GetInterface();
    
    {
        const auto& context = gi.GetNewContext( 0, false, L"Prepare Shadow Intersection Textures" );
        const auto& primitive = context.GetPointers();
        primitive.commandList->SoftReset();
        for (int i = 0; i < lights->size(); ++i)
        {
            gi.TransitTo(&primitive, component->m_shadow_mask_texs_[i].get(), BIND_TYPE_RTV);
            gi.TransitTo(&primitive, component->m_shadow_texs_[i].get(), BIND_TYPE_DSV);
        }
        primitive.commandList->FlagReady();
    }
    
	// First Pass: Shadow Map (With object and without object)
	// Draw shadow map except the object itself.
	for (int i = 0; i < lights->size(); ++i)
	{
		Graphics::SBs::LocalParamSB local_param{};
		local_param.SetParam<int>(shadow_slot, i);

		Managers::Renderer::GetInstance().RenderPassVanillaInclusion<SHADER_DOMAIN_OPAQUE, Engine::ForwardRenderPassTask>(
		    dt,
		    true,
		    local_param,
		    {
		        &Managers::RenderPipeline::GetInstance().GetLightSB(),
		        &Managers::ShadowManager::GetInstance().GetLightVPBuffer()
		    },
		    [this, &pivot](const Strong<Abstracts::ObjectBase>& obj)
             {
                 if (obj->GetID() == pivot->GetID())
                 {
                     return false;
                 }

                 if (obj->GetLayer() == RESERVED_LAYER_CAMERA || obj->GetLayer() == RESERVED_LAYER_UI ||
                     obj->GetLayer() == RESERVED_LAYER_ENVIRONMENT ||
                     obj->GetLayer() == RESERVED_LAYER_LIGHT || obj->GetLayer() == RESERVED_LAYER_SKYBOX)
                 {
                     return false;
                 }

                 return true;
             },
             [&gi, this, i, &component](const IGraphicContext* context)
             {
                 gi.SetViewport( context, m_viewport_ );
                 gi.BindGraphic( context, m_shadow_shader_.get() );
                 Resources::Texture* tmp_arr[] = { component->m_shadow_mask_texs_[i].get() };
                 gi.BindMultiple( context, tmp_arr, 1, component->m_shadow_texs_[i].get() );
             }, {}, {}, {}
		);
	}

	// Render object only for picking up the exact shadow position.
	for (int i = 0; i < lights->size(); ++i)
	{
	    Graphics::SBs::LocalParamSB local_param{};
	    local_param.SetParam<int>(shadow_slot, i);

	    Managers::Renderer::GetInstance().RenderPassVanillaInclusion<SHADER_DOMAIN_OPAQUE, Engine::ForwardRenderPassTask>(
            dt,
            true,
            local_param,
            {
                &Managers::RenderPipeline::GetInstance().GetLightSB(),
                &Managers::ShadowManager::GetInstance().GetLightVPBuffer()
            },
            [this, &pivot](const Strong<Abstracts::ObjectBase>& obj)
             {
                 if (obj->GetID() != pivot->GetID())
                 {
                     return false;
                 }

                 if (obj->GetLayer() == RESERVED_LAYER_CAMERA || obj->GetLayer() == RESERVED_LAYER_UI ||
                     obj->GetLayer() == RESERVED_LAYER_ENVIRONMENT ||
                     obj->GetLayer() == RESERVED_LAYER_LIGHT || obj->GetLayer() == RESERVED_LAYER_SKYBOX)
                 {
                     return false;
                 }

                 return true;
             },
             [&gi, this, i, &component](const IGraphicContext* context)
             {
                 gi.SetViewport( context, m_viewport_ );
                 gi.BindGraphic( context, m_shadow_shader_.get() );
                 Resources::Texture* tmp_arr[] = { component->m_shadow_mask_texs_[i].get() };
                 gi.BindMultiple( context, tmp_arr, 1, component->m_shadow_texs_[i].get() );
             }, {}, {}, {}
        );
	}

    {
        const auto& context = gi.GetNewContext( 0, false, L"Transit back Shadow Intersection Textures" );
        const auto& primitive = context.GetPointers();
        primitive.commandList->SoftReset();
        for (int i = 0; i < lights->size(); ++i)
        {
            gi.TransitBack(&primitive, component->m_shadow_mask_texs_[i].get(), BIND_TYPE_RTV);
            gi.TransitBack(&primitive, component->m_shadow_texs_[i].get(), BIND_TYPE_DSV);
        }
        primitive.commandList->FlagReady();
    }
}

void ShadowIntersectionRenderTask::SecondPass( const float dt,
    const ShadowIntersectionComponent* component,
    const std::vector<Graphics::SBs::LightVPSB> &light_vps,
    const Strong<Scene> &scene,
    const Strong<Layer> &lights) const
{
    // Second Pass: Intensity test
	// In light VP Render objects,
	// In pixel shader, while sampling shadow factor, do not use own shadow map
	// If shadow factor is above 0.f, and light intensity is above 0.f
	// then it intersects with light and shadow
	// Fill the pixel with light index (Do not use the sampler state, need raw value)
	constexpr size_t target_light_slot = 2;
	constexpr size_t custom_vp_slot    = 3;
	constexpr size_t custom_view_slot  = 1;
	constexpr size_t custom_proj_slot  = 2;

    IGraphicAPI& gi = g_graphic_accessor.GetInterface();
	{
	    const auto& context = gi.GetNewContext( 0, false, L"Intensity Render Prepare" );
	    const auto& primitive = context.GetPointers();
	    
	    primitive.commandList->SoftReset();
	    for (const auto& tex : component->m_shadow_texs_)
	    {
	        gi.TransitTo(&primitive, tex.get(), BIND_TYPE_SRV);
	    }
	    for (auto& tex : component->m_shadow_mask_texs_)
	    {
	        gi.TransitTo(&primitive, tex.get(), BIND_TYPE_SRV);
	    }
	    primitive.commandList->FlagReady();
	}

	// Find the nearest clip space for z value.
	int z_clip = 0;

	if (const auto camera = scene->GetMainCamera().lock())
	{
		const float z = camera->GetComponent<Components::Transform>().lock()->GetWorldPosition().z;

		for (int i = 0; i < 3; ++i)
		{
			if (light_vps[0].end_clip_spaces[i].z > z)
			{
				z_clip = i;
				break;
			}
		}
	}

    Abstracts::ObjectBase* pivot = component->GetOwner().lock().get();

	for (int i = 0; i < lights->size(); ++i)
	{
		Graphics::SBs::LocalParamSB local_param{};
		local_param.SetParam<int>(target_light_slot, i);
		local_param.SetParam<int>(custom_vp_slot, true);
		local_param.SetParam<Matrix>(custom_view_slot, light_vps[i].view[z_clip]);
		local_param.SetParam<Matrix>(custom_proj_slot, light_vps[i].proj[z_clip]);

        Managers::Renderer::GetInstance().RenderPassVanillaInclusion<SHADER_DOMAIN_OPAQUE, Engine::ForwardRenderPassTask>( 
                  dt,
                  true,
                  local_param,
                  { &Managers::RenderPipeline::GetInstance().GetLightSB(),
                    &Managers::ShadowManager::GetInstance().GetLightVPBuffer() },
                  [this, &pivot]( const Strong<Abstracts::ObjectBase> &obj )
                  {
                      if ( obj->GetID() == pivot->GetID() )
                      {
                          return false;
                      }

                      return true;
                  },
                  [this, i, &gi, &component]( const IGraphicContext *context )
                  {
                      gi.SetViewport( context, m_viewport_ );
                      gi.BindGraphic( context, m_intensity_test_shader_.get() );

                      Resources::Texture *rtvs[ ]{ component->m_intensity_test_texs_[ i ].get(),
                                                   component->m_intensity_position_texs_[ i ].get() };

                      gi.BindMultiple( context, rtvs, 2, m_tmp_shadow_depth_.get() );
                      gi.BindMultiple
                              ( context,
                                component->m_shadow_texs_raw_.data(),
                                BIND_TYPE_SRV,
                                BIND_SLOT_TEX,
                                0,
                                component->m_shadow_texs_raw_.size() );
                      gi.BindMultiple
                              ( context,
                                component->m_shadow_mask_texs_raw_.data(),
                                BIND_TYPE_SRV,
                                RESERVED_TEX_SHADOW_MAP,
                                0,
                                CFG_MAX_DIRECTIONAL_LIGHT );
                  },
                  [this, i, &component, &gi]( const IGraphicContext *context )
                  {
                      Resources::Texture *rtvs[ ]{ component->m_intensity_test_texs_[ i ].get(),
                                                   component->m_intensity_position_texs_[ i ].get() };

                      gi.TransitBackMultiple( context, rtvs, 2, BIND_TYPE_RTV );
                  },
                  {},
                  {} );
	}

    {
	    const auto& context = gi.GetNewContext( 0, false, L"Intensity Render Cleanup" );
	    const auto& primitive = context.GetPointers();
	    
	    primitive.commandList->SoftReset();
	    gi.Clear( &primitive, m_tmp_shadow_depth_.get(), BIND_TYPE_DSV );
	    for (const auto& tex : component->m_shadow_texs_)
	    {
	        gi.TransitBack(&primitive, tex.get(), BIND_TYPE_SRV);
	    }
	    for (auto& tex : component->m_shadow_mask_texs_)
	    {
	        gi.TransitBack(&primitive, tex.get(), BIND_TYPE_SRV);
	    }
	    primitive.commandList->FlagReady();
    }
}

void ShadowIntersectionRenderTask::ThirdPass( const float dt, const ShadowIntersectionComponent* component, const Strong<Layer> &lights ) const
{
    // Third Pass: Intersection Compute
    // By compute shader, For each pixel if it has light index, which is non-zero,
    // then this object shadow intersects with designated light.
    static constexpr UINT group[] = {256, 1, 1};
    
    IGraphicAPI& gi = g_graphic_accessor.GetInterface();
    const auto& context = gi.GetNewContext( 0, true, L"Intersection Compute Dispatch" );
    const auto& primitive = context.GetPointers();

    primitive.commandList->SoftReset();
    for (int i = 0; i < lights->size(); ++i)
    {
        const auto& cast            = m_intersection_compute_->GetSharedPtr<ShadowIntensityComputeShader>();
        
        cast->SetIntersectionTexture(component->m_intensity_test_texs_[i]);
        cast->SetPositionTexture(component->m_intensity_position_texs_[i]);
        cast->SetLightTable(component->m_sb_light_table_);
        cast->SetTargetLight(i);
        Graphics::SBs::LocalParamSB empty_param{};
        cast->Dispatch(&primitive, group, empty_param, dt);
    }
    primitive.commandList->Execute();
}

void ShadowIntersectionRenderTask::PreRun( const RenderMap *render_map,
        const size_t render_map_count,
        const ObjectPredication &predication )
{}
#endif