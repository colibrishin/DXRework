using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class RaytracingRenderPassTask : CommonProject
{
    public RaytracingRenderPassTask() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<CoreRender>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<TBB>(target);
        conf.AddPublicDependency<EngineEntryPoint>(target);

        conf.AddPublicDependency<RenderPipeline>(target);
        conf.AddPublicDependency<RaytracingShader>(target);
        conf.AddPublicDependency<Material>(target);
        conf.AddPublicDependency<Mesh>(target);
        conf.AddPrivateDependency<Shape>(target);
        conf.AddPrivateDependency<Texture>(target);
        conf.AddPrivateDependency<AtlasAnimationTexture>(target);
        conf.AddPrivateDependency<AnimationTexture>(target);
    }
}