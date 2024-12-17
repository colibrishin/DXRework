using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class Texture : CommonProject
{
    public Texture() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<RenderPipeline>(target);

        conf.AddPrivateDependency<ResourceManager>(target);
        conf.AddPrivateDependency<ThrowIfFailed>(target);
    }
}