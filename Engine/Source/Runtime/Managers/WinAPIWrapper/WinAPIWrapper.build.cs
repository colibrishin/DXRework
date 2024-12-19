using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class WinAPIWrapper : CommonProject
{
    public WinAPIWrapper() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPrivateDependency<Core>(target);
        conf.AddPrivateDependency<Boost>(target);
        conf.AddPrivateDependency<EngineEntryPoint>(target);
    }
}