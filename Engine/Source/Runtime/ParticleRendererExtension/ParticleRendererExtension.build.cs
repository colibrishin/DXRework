using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class ParticleRendererExtension : EngineCommonProject
{
    public ParticleRendererExtension() 
    {
        SourceFilesExtensions.Add(".hlsl");
        SourceFilesExtensions.Add(".png");
        SourceFilesExtensions.Add(".xml");
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<TBB>(target);
        conf.AddPublicDependency<ParticleRenderer>(target);
        conf.AddPublicDependency<Texture2D>(target);
        conf.AddPublicDependency<AtlasAnimation>(target);
        conf.AddPublicDependency<AtlasAnimationTexture>(target);

        conf.TargetCopyFiles.Add
        (
            @"cs_particle.hlsl",
            @"noise0.png",
            @"noise1.png",
            @"noise2.png",
            @"water-vortex/water-vortex.png",
            @"water-vortex/water-vortex.xml"
        );
    }
}