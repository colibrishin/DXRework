using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/Boost/Boost.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DX12Agility/DX12Agility.build.cs")]

[Generate]
public class ParticleRendererRenderTask : EngineCommonProject
{
    public ParticleRendererRenderTask() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<TBB>(target);
        conf.AddPublicDependency<EngineEntryPoint>(target);
        conf.AddPublicDependency<RenderComponent>(target);

        conf.AddPrivateDependency<Mesh>(target);
        conf.AddPrivateDependency<Shader>(target);
        conf.AddPrivateDependency<Material>(target);
        conf.AddPrivateDependency<ParticleRenderer>(target);
        conf.AddPrivateDependency<ParticleRendererExtension>(target);
        conf.AddPrivateDependency<AnimationTexture>(target);
        conf.AddPrivateDependency<AtlasAnimationTexture>(target);
        conf.AddPrivateDependency<Animator>(target);
    }
}