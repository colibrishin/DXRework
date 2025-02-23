using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class CoreTaskScheduler : EngineCommonProject
{
    public CoreTaskScheduler() {}

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<CoreSingleton>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPrivateDependency<EngineEntryPoint>(target);
    }
}