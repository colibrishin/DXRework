using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class CoreRender : CommonProject
{
    public CoreRender() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<TBB>(target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<CoreType>(target);
        conf.AddPublicDependency<CoreEntity>(target);
    }
}