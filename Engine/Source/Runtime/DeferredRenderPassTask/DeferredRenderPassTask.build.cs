using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class DeferredRenderPassTask : EngineCommonProject
{
    public DeferredRenderPassTask() 
    {
        ResourceFilesExtensions.Add(".hlsl");
        ResourceFilesExtensions.Add(".hlsli");
        ResourceFilesExtensions.Add(".json");
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<TBB>(target);
        conf.AddPublicDependency<Shader>(target);
        conf.AddPublicDependency<Texture>(target);
        conf.AddPublicDependency<RenderPipeline>(target);
        conf.AddPublicDependency<Material>(target);
        conf.AddPublicDependency<Mesh>(target);

        conf.AddPrivateDependency<Shape>(target);
        conf.AddPrivateDependency<AtlasAnimationTexture>(target);
        conf.AddPrivateDependency<AnimationTexture>(target);

        conf.TargetCopyFiles.Add
        (
            @"deferred_default_firstpass.hlsl",
            @"deferred_pbr_secondpass.hlsl",
            @"deferred_secondpass.hlsl"
        );
    }
}