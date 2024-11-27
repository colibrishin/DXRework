using System;
using System.Collections;
using Microsoft.Win32;
using Sharpmake;

[module: Include("%EngineDir%/Build/ExportProject.build.cs")]

[Sharpmake.Export]
public class DirectXTK : VCPKG
{
    public DirectXTK() : base()
    {
        Name = "DirectXMath";
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
            @"DirectX-Guids.lib", 
            @"DirectX-Headers.lib", 
            @"DirectXTK12.lib",
            @"dxcompiler.lib"
        );
    }

    public override void ConfigureRelease(Configuration conf, EngineTarget target)
    {
        base.ConfigureRelease(conf, target);

        conf.LibraryFiles.Add
        (
            @"DirectX-Guids.lib", 
            @"DirectX-Headers.lib", 
            @"DirectXTK12.lib",
            @"dxcompiler.lib"
        );
    }
}