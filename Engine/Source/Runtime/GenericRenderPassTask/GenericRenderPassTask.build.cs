using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class GenericRenderPassTask : CommonProject
{
    public GenericRenderPassTask() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<TBB>(target);
        conf.AddPublicDependency<EngineEntryPoint>(target);

        conf.AddPublicDependency<RenderPipeline>(target);
        conf.AddPublicDependency<Shader>(target);
        conf.AddPublicDependency<Material>(target);
        conf.AddPrivateDependency<Shape>(target);
        conf.AddPrivateDependency<Texture>(target);
        conf.AddPrivateDependency<AtlasAnimationTexture>(target);
        conf.AddPrivateDependency<AnimationTexture>(target);
    }
}