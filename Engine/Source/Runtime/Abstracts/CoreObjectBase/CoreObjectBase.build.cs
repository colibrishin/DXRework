using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Abstracts/CoreActor/CoreActor.build.cs")]

[Generate]
public class CoreObjectBase : CommonProject
{
    public CoreObjectBase() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<TypeLibrary>(target);
        conf.AddPublicDependency<CoreComponent>(target);
        conf.AddPublicDependency<CoreActor>(target);
        conf.AddPublicDependency<Delegation>(target);
        conf.AddPublicDependency<Scene>(target);
    }
}