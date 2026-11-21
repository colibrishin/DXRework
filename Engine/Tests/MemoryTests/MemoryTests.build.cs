using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Memory/Memory.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/GTest/GTest.build.cs")]

// Unit tests of the object pools and managed pointers (Memory.dll), without the engine.
// Run Binaries/<configuration>/MemoryTests.exe; the exit code is non-zero when a test fails.
[Generate]
public class MemoryTests : EngineCommonProject
{
    public MemoryTests() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.SolutionFolder = @"Tests";

        conf.AddPrivateDependency<Memory>(target);
        conf.AddPrivateDependency<GTest>(target);

        conf.Options.Add(Options.Vc.Linker.SubSystem.Console);
        conf.Output = Configuration.OutputType.Exe;
    }
}
