using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Memory/Memory.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/Boost/Boost.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Core/Core.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/GTest/GTest.build.cs")]

// Unit tests of Core building blocks (Delegate, ...) against Core.dll, without starting the engine.
// Run Binaries/<configuration>/CoreTests.exe; the exit code is non-zero when a test fails.
[Generate]
public class CoreTests : EngineCommonProject
{
    public CoreTests() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.SolutionFolder = @"Tests";

        // Linked directly: Core's public dependencies do not pass their libraries on to an executable.
        conf.AddPrivateDependency<Memory>(target);
        conf.AddPrivateDependency<Boost>(target);
        conf.AddPrivateDependency<Core>(target);
        conf.AddPrivateDependency<GTest>(target);

        conf.Options.Add(Options.Vc.Linker.SubSystem.Console);
        conf.Output = Configuration.OutputType.Exe;
    }
}
