#include "ParticleRendererExtensionModule.h"
#include "ParticleRendererExtensionModule.generated.h"

#include "AtlasAnimation.h"
#include "AtlasAnimationTexture.h"

#include "SimpleParticleComputeShader.h"
#include "Texture2D.h"

#include "ModuleManager/Public/ModuleManager.h"

MODULE_IMPL(Engine::ParticleRendererExtensionModule, ParticleRendererExtension)

bool Engine::ParticleRendererExtensionModule::InitializeImpl()
{
    Resources::SimpleParticleComputeShader::Create("SimpleParticleComputeShader");
    
    const Strong<Resources::AtlasAnimation>& anim = Resources::AtlasAnimation::Create(
        "water-vortex",
        "water-vortex.xml" );
    const Strong<Resources::Texture2D>& anim_tex = Resources::Texture2D::Create(
        "water-vortex",
        "water-vortex.png",
        GenericTextureDescription{} );
    Resources::AtlasAnimationTexture::Create("water-vortex", "", std::vector{anim}, std::vector{anim_tex});

    return true;
}

bool Engine::ParticleRendererExtensionModule::ShutdownImpl()
{
    return true;
}

bool Engine::ParticleRendererExtensionModule::DynamicLoadable()
{
    return true;
}
