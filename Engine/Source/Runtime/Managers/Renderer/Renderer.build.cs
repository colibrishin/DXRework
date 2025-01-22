using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Abstracts/CoreSingleton/CoreSingleton.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/ConcurrnetTypeLibrary/ConcurrnetTypeLibrary.build.cs")]

[Generate]
public class TaskScheduler : CommonProject
{
    public TaskScheduler() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<TypeLibrary>(target);
        conf.AddPublicDependency<CoreSingleton>(target);
    }
}