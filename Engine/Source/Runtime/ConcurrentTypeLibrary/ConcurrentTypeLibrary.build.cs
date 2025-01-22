using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/TBB/TBB.build.cs")]

[Generate]
public class ConcurrentTypeLibrary : CommonProject
{
    public ConcurrentTypeLibrary() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPrivateDependency<TypeLibrary>(target);
        conf.AddPublicDependency<Allocator>(target);
        conf.AddPublicDependency<TBB>(target);
    }
}