using System;
using System.Collections;
using Microsoft.Win32;
using Sharpmake;

[module: Include("%EngineDir%/Build/ExportProject.build.cs")]

[Sharpmake.Export]
public class DX12Agility : VCPKG
{
    public DX12Agility() : base()
    {
        Name = "DX12Agility";
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
            @"d3d12.lib",
            @"dxguid.lib",
            @"dxgi.lib",
            @"d3dcompiler.lib",
            @"dxcompiler.lib",
            @"DirectX-Guids.lib", 
            @"DirectX-Headers.lib", 
            @"dxcompiler.lib"
        );
    }

    public override void ConfigureRelease(Configuration conf, EngineTarget target)
    {
        base.ConfigureRelease(conf, target);

        conf.LibraryFiles.Add
        (
            @"d3d12.lib",
            @"dxguid.lib",
            @"dxgi.lib",
            @"d3dcompiler.lib",
            @"dxcompiler.lib",
            @"DirectX-Guids.lib", 
            @"DirectX-Headers.lib", 
            @"dxcompiler.lib"
        );
    }
}