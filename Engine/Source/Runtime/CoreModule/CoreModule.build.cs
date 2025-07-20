using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/Boost/Boost.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/TBB/TBB.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/GCEM/GCEM.build.cs")]

[Generate]
public class CoreModule : EngineCommonProject
{
    public CoreModule() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<CoreType>(target);
        conf.AddPublicDependency<Boost>(target);
    }
}