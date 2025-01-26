using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class ParticleRendererExtension : CommonProject
{
    public ParticleRendererExtension() 
    {
        SourceFilesExtensions.Add(".hlsl");
        SourceFilesExtensions.Add(".png");
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<TBB>(target);
        conf.AddPublicDependency<ParticleRenderer>(target);

        conf.TargetCopyFiles.Add
        (
            @"cs_particle.hlsl",
            @"noise0.png",
            @"noise1.png",
            @"noise2.png"
        );
    }
}