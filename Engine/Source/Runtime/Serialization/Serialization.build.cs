using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Abstracts/CoreEntity/CoreEntity.build.cs")]

[Generate]
public class Serialization : CommonProject
{
    public Serialization() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<TypeLibrary>(target);
        conf.AddPublicDependency<CoreEntity>(target);
    }
}