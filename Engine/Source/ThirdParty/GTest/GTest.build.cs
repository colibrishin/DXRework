using System;
using System.Collections;
using Microsoft.Win32;
using Sharpmake;

[module: Include("%EngineDir%/Build/ExportProject.build.cs")]

// GoogleTest from vcpkg (dynamic), for the test programs only.
[Sharpmake.Export]
public class GTest : VCPKG
{
    public GTest() : base()
    {
        Name = "GTest";
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.ExportDefines.Add("GTEST_LINKED_AS_SHARED_LIBRARY=1");
    }

    public override void ConfigureDebug(Configuration conf, EngineTarget target)
    {
        base.ConfigureDebug(conf, target);
        conf.LibraryFiles.Add(@"gtest.lib");
        conf.TargetCopyFiles.Add(GetVCPKGBinPath(target) + @"/gtest.dll");
    }

    public override void ConfigureRelease(Configuration conf, EngineTarget target)
    {
        base.ConfigureRelease(conf, target);
        conf.LibraryFiles.Add(@"gtest.lib");
        conf.TargetCopyFiles.Add(GetVCPKGBinPath(target) + @"/gtest.dll");
    }
}
