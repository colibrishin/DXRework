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
            @"boost_atomic-vc143-mt-gd-x64-1_86.lib", 
            @"boost_chrono-vc143-mt-gd-x64-1_86.lib", 
            @"boost_container-vc143-mt-gd-x64-1_86.lib",
            @"boost_date_time-vc143-mt-gd-x64-1_86.lib",
            @"boost_regex-vc143-mt-gd-x64-1_86.lib",
            @"boost_serialization-vc143-mt-gd-x64-1_86.lib",
            @"boost_system-vc143-mt-gd-x64-1_86.lib",
            @"boost_thread-vc143-mt-gd-x64-1_86.lib",
            @"boost_wserialization-vc143-mt-gd-x64-1_86.lib",
            @"boost_filesystem-vc143-mt-gd-x64-1_86.lib"
        );
    }

    public override void ConfigureRelease(Configuration conf, EngineTarget target)
    {
        base.ConfigureRelease(conf, target);

        conf.LibraryFiles.Add
        (
            @"boost_atomic-vc143-mt-x64-1_86.lib", 
            @"boost_chrono-vc143-mt-x64-1_86.lib", 
            @"boost_container-vc143-mt-x64-1_86.lib",
            @"boost_date_time-vc143-mt-x64-1_86.lib",
            @"boost_regex-vc143-mt-x64-1_86.lib",
            @"boost_serialization-vc143-mt-x64-1_86.lib",
            @"boost_system-vc143-mt-x64-1_86.lib",
            @"boost_thread-vc143-mt-x64-1_86.lib",
            @"boost_wserialization-vc143-mt-x64-1_86.lib",
            @"boost_filesystem-vc143-mt-x64-1_86.lib"
        );
    }
}