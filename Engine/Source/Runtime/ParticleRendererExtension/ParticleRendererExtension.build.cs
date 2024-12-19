using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class ParticleRendererExtension : CommonProject
{
    public ParticleRendererExtension() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<TBB>(target);
        conf.AddPublicDependency<ParticleRenderer>(target);
    }
}