using System;
using System.Collections;
using Microsoft.Win32;
using Sharpmake;

[module: Include("%EngineDir%/Build/ExportProject.build.cs")]

[Sharpmake.Export]
public class Boost : VCPKG
{
    public Boost() : base()
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
            @"boost_atomic-vc140-mt-gd.lib", 
            @"boost_chrono-vc140-mt-gd.lib", 
            @"boost_container-vc140-mt-gd.lib",
            @"boost_date_time-vc140-mt-gd.lib",
            @"boost_regex-vc140-mt-gd.lib",
            @"boost_serialization-vc140-mt-gd.lib",
            @"boost_system-vc140-mt-gd.lib",
            @"boost_thread-vc140-mt-gd.lib",
            @"boost_wserialization-vc140-mt-gd.lib"
        );
    }

    public override void ConfigureRelease(Configuration conf, EngineTarget target)
    {
        base.ConfigureRelease(conf, target);

        conf.LibraryFiles.Add
        (
            @"boost_atomic-vc140-mt.lib", 
            @"boost_chrono-vc140-mt.lib", 
            @"boost_container-vc140-mt.lib",
            @"boost_date_time-vc140-mt.lib",
            @"boost_regex-vc140-mt.lib",
            @"boost_serialization-vc140-mt.lib",
            @"boost_system-vc140-mt.lib",
            @"boost_thread-vc140-mt.lib",
            @"boost_wserialization-vc140-mt.lib"
        );
    }
}