using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class ParticleRenderer : CommonProject
{
    public ParticleRenderer() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<RenderComponent>(target);
        conf.AddPublicDependency<ComputeShader>(target);
        conf.AddPublicDependency<DX12Agility>(target);
        conf.AddPublicDependency<DirectXTK>(target);

        conf.AddPrivateDependency<RenderPipeline>(target);
    }
}