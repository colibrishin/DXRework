using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class DeferredRenderPassTask : EngineCommonProject
{
    public DeferredRenderPassTask() 
    {
        SourceFilesExtensions.Add(".hlsl");
        SourceFilesExtensions.Add(".hlsli");
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<TBB>(target);
        conf.AddPublicDependency<Shader>(target);
        conf.AddPublicDependency<Texture2D>(target);
        conf.AddPublicDependency<RenderPipeline>(target);
        conf.AddPublicDependency<Material>(target);

        conf.AddPrivateDependency<Shape>(target);
        conf.AddPrivateDependency<Texture>(target);
        conf.AddPrivateDependency<AtlasAnimationTexture>(target);
        conf.AddPrivateDependency<AnimationTexture>(target);

        conf.TargetCopyFiles.Add(@"deferred_default_firstpass.hlsl", @"deferred_secondpass.hlsl");
    }
}