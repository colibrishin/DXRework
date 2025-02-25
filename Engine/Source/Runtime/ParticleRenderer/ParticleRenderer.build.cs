using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class ParticleRenderer : EngineCommonProject
{
    public ParticleRenderer() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<ShapeRenderComponent>(target);
        conf.AddPublicDependency<ComputeShader>(target);
        conf.AddPrivateDependency<RenderPipeline>(target);
    }
}