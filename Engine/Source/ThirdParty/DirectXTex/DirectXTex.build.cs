using System;
using System.Collections;
using Microsoft.Win32;
using Sharpmake;

[module: Include("%EngineDir%/Build/ExportProject.build.cs")]

[Sharpmake.Export]
public class DirectXTex : VCPKG
{
    public DirectXMath() : base()
    {
        Name = "DirectXTex";
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target) 
    {
        base.ConfigureAll(conf, target);
    }
}