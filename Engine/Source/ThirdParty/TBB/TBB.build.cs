using System;
using System.Collections;
using Microsoft.Win32;
using Sharpmake;

[module: Include("%EngineDir%/Build/ExportProject.build.cs")]

[Sharpmake.Export]
public class TBB : VCPKG
{
    public TBB() : base()
    {
        Name = "TBB";
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
            @"tbb12_debug.lib", 
            @"tbbmalloc_debug.lib", 
            @"tbbmalloc_proxy_debug.lib",
	        @"hwloc.lib"
        );

        string BinPath = GetVCPKGBinPath(target);

        conf.TargetCopyFiles.Add
        (
            BinPath + @"/tbb12_debug.dll",
            BinPath + @"/tbbmalloc_debug.dll",
            BinPath + @"/tbbmalloc_proxy_debug.dll",
            BinPath + @"/hwloc-15.dll"
        );
    }

    public override void ConfigureRelease(Configuration conf, EngineTarget target)
    {
        base.ConfigureRelease(conf, target);

        conf.LibraryFiles.Add
        (
            @"tbb12.lib", 
            @"tbbmalloc.lib", 
            @"tbbmalloc_proxy.lib",
	        @"hwloc.lib"
        );

        string BinPath = GetVCPKGBinPath(target);

        conf.TargetCopyFiles.Add
        (
            BinPath + @"/tbb12.dll",
            BinPath + @"/tbbmalloc.dll",
            BinPath + @"/tbbmalloc_proxy.dll",
            BinPath + @"/hwloc-15.dll"
        );
    }
}