#include "DeferredRenderPassTaskModule.h"
#include "DeferredRenderPassTaskModule.generated.h"

#include "DeferredRenderPassTask.h"
#include "Renderer.h"
#include "Shader.h"
#include "Texture2D.h"

MODULE_IMPL( Engine::DeferredRenderPassTaskModule, DeferredRenderPassTask );

bool Engine::DeferredRenderPassTaskModule::InitializeImpl()
{
#ifdef CFG_RENDERTYPE_DEFERRED
    DeferredRenderPassTaskFactory* factory = new DeferredRenderPassTaskFactory();
    const std::string name ( DeferredRenderPassTask::StaticTypeName() );
    const std::wstring name_wstr( name.begin(), name.end() );

    Managers::Renderer::GetInstance().RegisterRenderPass( name_wstr, factory );
    Managers::Renderer::GetInstance().RenderPassWith( name_wstr, SHADER_DOMAIN_OPAQUE );

    Resources::Shader::Create( "DeferredMaterialPass",
                               "deferred_default_firstpass.hlsl",
                               SHADER_DOMAIN_OPAQUE,
                               true,
                               SHADER_DEPTH_TEST_ALL,
                               SHADER_DEPTH_LESS,
                               SHADER_SAMPLER_CLAMP,
                               SHADER_SAMPLER_LESS_EQUAL,
                               SAMPLER_FILTER_MIN_MAG_MIP_POINT,
                               SHADER_RASTERIZER_CULL_BACK,
                               SHADER_RASTERIZER_FILL_SOLID,
                               std::vector{ TEX_FORMAT_R32G32B32A32_FLOAT,
                                            TEX_FORMAT_R8G8B8A8_UNORM,
                                            TEX_FORMAT_R8G8B8A8_UNORM,
                                            TEX_FORMAT_R32G32B32A32_FLOAT },
                               TEX_FORMAT_D32_FLOAT );

    factory->SetLightShader( Resources::Shader::Create( "DeferredLightPass",
                                                     "deferred_secondpass.hlsl",
                                                     SHADER_DOMAIN_OPAQUE,
                                                     true,
                                                     SHADER_DEPTH_TEST_ALL,
                                                     SHADER_DEPTH_LESS,
                                                     SHADER_SAMPLER_CLAMP,
                                                     SHADER_SAMPLER_LESS_EQUAL,
                                                     SAMPLER_FILTER_MIN_MAG_MIP_POINT,
                                                     SHADER_RASTERIZER_CULL_BACK,
                                                     SHADER_RASTERIZER_FILL_SOLID,
                                                     GetDefaultRTVFormat() ) );

    factory->SetTexture(
            Resources::Texture2D::Create( "DeferredA",
                                          "",
                                          GenericTextureDescription{ .Dimension        = TEX_TYPE_2D,
                                                                     .Alignment        = 0,
                                                                     .Width            = CFG_WIDTH,
                                                                     .Height           = CFG_HEIGHT,
                                                                     .DepthOrArraySize = 1,
                                                                     .Format           = TEX_FORMAT_R32G32B32A32_FLOAT,
                                                                     .Flags            = RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                                                     .MipsLevel        = 1,
                                                                     .Layout           = TEX_LAYOUT_UNKNOWN,
                                                                     .SampleDesc       = { .Count = 1, .Quality = 0 },
                                                                     .AsSRV            = true,
                                                                     .AsRTV            = true } ),
            0 );

    factory->SetTexture(
            Resources::Texture2D::Create( "DeferredB",
                                          "",
                                          GenericTextureDescription{ .Dimension        = TEX_TYPE_2D,
                                                                     .Alignment        = 0,
                                                                     .Width            = CFG_WIDTH,
                                                                     .Height           = CFG_HEIGHT,
                                                                     .DepthOrArraySize = 1,
                                                                     .Format           = TEX_FORMAT_R8G8B8A8_UNORM,
                                                                     .Flags            = RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                                                     .MipsLevel        = 1,
                                                                     .Layout           = TEX_LAYOUT_UNKNOWN,
                                                                     .SampleDesc       = { .Count = 1, .Quality = 0 },
                                                                     .AsSRV            = true,
                                                                     .AsRTV            = true } ),
            1 );

    factory->SetTexture(
            Resources::Texture2D::Create( "DeferredC",
                                          "",
                                          GenericTextureDescription{ .Dimension        = TEX_TYPE_2D,
                                                                     .Alignment        = 0,
                                                                     .Width            = CFG_WIDTH,
                                                                     .Height           = CFG_HEIGHT,
                                                                     .DepthOrArraySize = 1,
                                                                     .Format           = TEX_FORMAT_R8G8B8A8_UNORM,
                                                                     .Flags            = RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                                                     .MipsLevel        = 1,
                                                                     .Layout           = TEX_LAYOUT_UNKNOWN,
                                                                     .SampleDesc       = { .Count = 1, .Quality = 0 },
                                                                     .AsSRV            = true,
                                                                     .AsRTV            = true } ),
            2 );

    factory->SetTexture(
            Resources::Texture2D::Create( "DeferredD",
                                          "",
                                          GenericTextureDescription{ .Dimension        = TEX_TYPE_2D,
                                                                     .Alignment        = 0,
                                                                     .Width            = CFG_WIDTH,
                                                                     .Height           = CFG_HEIGHT,
                                                                     .DepthOrArraySize = 1,
                                                                     .Format           = TEX_FORMAT_R32G32B32A32_FLOAT,
                                                                     .Flags            = RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                                                     .MipsLevel        = 1,
                                                                     .Layout           = TEX_LAYOUT_UNKNOWN,
                                                                     .SampleDesc       = { .Count = 1, .Quality = 0 },
                                                                     .AsSRV            = true,
                                                                     .AsRTV            = true } ),
            3 );

    factory->SetDepthStencil(
            Resources::Texture2D::Create( "DeferredDepth",
                                          "",
                                          GenericTextureDescription{ .Dimension        = TEX_TYPE_2D,
                                                                     .Alignment        = 0,
                                                                     .Width            = CFG_WIDTH,
                                                                     .Height           = CFG_HEIGHT,
                                                                     .DepthOrArraySize = 1,
                                                                     .Format           = TEX_FORMAT_R32_TYPELESS,
                                                                     .Flags      = RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
                                                                     .MipsLevel  = 1,
                                                                     .Layout     = TEX_LAYOUT_UNKNOWN,
                                                                     .SampleDesc = { .Count = 1, .Quality = 0 },
                                                                     .AsSRV      = true,
                                                                     .AsDSV      = true,
                                                                     .Srv = {
					                                                     .Format = TEX_FORMAT_R32_FLOAT,
					                                                     .ViewDimension = SRV_DIMENSION_TEXTURE2D,
					                                                     .Shader4ComponentMapping = d3d12_shader4_component_mapping,
					                                                     .Texture2D = {
                                                                             .MostDetailedMip = 0,
                                                                             .MipLevels = 1,
                                                                             .PlaneSlice = 0,
                                                                             .ResourceMinLODClamp = 0,
					                                                     },
				                                                      },
				                                                     .Dsv = {
					                                                     .Format = TEX_FORMAT_D32_FLOAT,
					                                                     .ViewDimension = DSV_DIMENSION_TEXTURE2D,
					                                                     .Flags = DSV_FLAG_NONE,
                                                                         .Texture2D = {
                                                                            .MipSlice = 0
                                                                         }
                                                                     } } ) );
#endif
    return true;
}

bool Engine::DeferredRenderPassTaskModule::ShutdownImpl()
{
#ifdef CFG_RENDERTYPE_DEFERRED
    const std::string  name( DeferredRenderPassTask::StaticTypeName() );
    const std::wstring name_wstr( name.begin(), name.end() );

    Managers::Renderer::GetInstance().RenderPassWithout( name_wstr, SHADER_DOMAIN_OPAQUE );
    Managers::Renderer::GetInstance().UnregisterRenderPass( name_wstr );
#endif
    return true;
}

bool Engine::DeferredRenderPassTaskModule::DynamicLoadable()
{
    return true;
}
