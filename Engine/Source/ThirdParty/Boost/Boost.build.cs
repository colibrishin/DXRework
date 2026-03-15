using System;
using System.Collections;
using Microsoft.Win32;
using Sharpmake;

[module: Include("%EngineDir%/Build/ExportProject.build.cs")]

[Sharpmake.Export]
public class Boost : VCPKG
{
    public Boost()
    {
        Name = "Boost";
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target) 
    {
        base.ConfigureAll(conf, target);
    }

    public override void ConfigureDebug(Configuration conf, EngineTarget target)
    {
        base.ConfigureDebug(conf, target);

        conf.LibraryFiles.Add
        (
            @"boost_atomic-vc145-mt-gd-x64-1_90.lib",
            @"boost_chrono-vc145-mt-gd-x64-1_90.lib",
            @"boost_container-vc145-mt-gd-x64-1_90.lib",
            @"boost_context-vc145-mt-gd-x64-1_90.lib",
            @"boost_date_time-vc145-mt-gd-x64-1_90.lib",
            @"boost_serialization-vc145-mt-gd-x64-1_90.lib",
            @"boost_thread-vc145-mt-gd-x64-1_90.lib",
            @"boost_wserialization-vc145-mt-gd-x64-1_90.lib"
        );

        string BinPath = GetVCPKGBinPath(target);

        conf.TargetCopyFiles.Add
        (
            BinPath + @"/boost_atomic-vc145-mt-gd-x64-1_90.dll",
            BinPath + @"/boost_chrono-vc145-mt-gd-x64-1_90.dll",
            BinPath + @"/boost_container-vc145-mt-gd-x64-1_90.dll",
            BinPath + @"/boost_context-vc145-mt-gd-x64-1_90.dll",
            BinPath + @"/boost_date_time-vc145-mt-gd-x64-1_90.dll",
            BinPath + @"/boost_serialization-vc145-mt-gd-x64-1_90.dll",
            BinPath + @"/boost_thread-vc145-mt-gd-x64-1_90.dll",
            BinPath + @"/boost_wserialization-vc145-mt-gd-x64-1_90.dll"
        );
    }

    public override void ConfigureRelease(Configuration conf, EngineTarget target)
    {
        base.ConfigureRelease(conf, target);

        conf.LibraryFiles.Add
        (
            @"boost_atomic-vc145-mt-x64-1_90.lib",
            @"boost_chrono-vc145-mt-x64-1_90.lib",
            @"boost_container-vc145-mt-x64-1_90.lib",
            @"boost_context-vc145-mt-x64-1_90.lib",
            @"boost_date_time-vc145-mt-x64-1_90.lib",
            @"boost_serialization-vc145-mt-x64-1_90.lib",
            @"boost_thread-vc145-mt-x64-1_90.lib",
            @"boost_wserialization-vc145-mt-x64-1_90.lib"
        );

        string BinPath = GetVCPKGBinPath(target);

        conf.TargetCopyFiles.Add
        (
            BinPath + @"/boost_atomic-vc145-mt-x64-1_90.dll",
            BinPath + @"/boost_chrono-vc145-mt-x64-1_90.dll",
            BinPath + @"/boost_container-vc145-mt-x64-1_90.dll",
            BinPath + @"/boost_context-vc145-mt-x64-1_90.dll",
            BinPath + @"/boost_date_time-vc145-mt-x64-1_90.dll",
            BinPath + @"/boost_serialization-vc145-mt-x64-1_90.dll",
            BinPath + @"/boost_thread-vc145-mt-x64-1_90.dll",
            BinPath + @"/boost_wserialization-vc145-mt-x64-1_90.dll"
        );
    }
}