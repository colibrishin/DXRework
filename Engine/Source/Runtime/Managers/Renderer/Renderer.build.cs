using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Core/Core.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/CommandPair/CommandPair.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/DescriptorHeap/DescriptorHeap.build.cs")]

[Generate]
public class TaskScheduler : CommonProject
{
    public TaskScheduler() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
    }
}