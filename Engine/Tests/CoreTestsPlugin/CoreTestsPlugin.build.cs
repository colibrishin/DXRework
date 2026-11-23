using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Memory/Memory.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/Boost/Boost.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Core/Core.build.cs")]

// A DLL that CoreTests loads and unloads to check that a module's listeners can be removed before it is unloaded.
[Generate]
public class CoreTestsPlugin : EngineCommonProject
{
    public CoreTestsPlugin() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.SolutionFolder = @"Tests";

        conf.AddPrivateDependency<Memory>(target);
        conf.AddPrivateDependency<Boost>(target);
        conf.AddPrivateDependency<Core>(target);

        conf.Output = Configuration.OutputType.Dll;
    }
}
