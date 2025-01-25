using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class Texture2D : CommonProject
{
    public Texture2D() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<TBB>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<Texture>(target);
    }
}