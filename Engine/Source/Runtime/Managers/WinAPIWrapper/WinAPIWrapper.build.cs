using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class WinAPIWrapper : EngineCommonProject
{
    public WinAPIWrapper() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPrivateDependency<Core>(target);
        conf.AddPrivateDependency<Boost>(target);
        conf.AddPublicDependency<CoreType>(target);
        conf.AddPrivateDependency<EngineEntryPoint>(target);
    }
}