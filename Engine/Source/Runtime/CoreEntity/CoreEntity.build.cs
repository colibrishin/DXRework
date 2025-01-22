using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/Boost/Boost.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/TBB/TBB.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DirectXTK/DirectXTK.build.cs")]

[Generate]
public class CoreEntity : CommonProject
{
    public CoreEntity() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<TBB>(target);
    }
}