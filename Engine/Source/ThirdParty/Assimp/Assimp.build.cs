using System;
using System.Collections;
using Microsoft.Win32;
using Sharpmake;

[module: Include("%EngineDir%/Build/ExportProject.build.cs")]

[Sharpmake.Export]
public class Assimp : VCPKG
{
    public Assimp() : base()
    {
        Name = "Assimp";
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target) 
    {
        base.ConfigureAll(conf, target);

        conf.LibraryFiles.Add
        (
            @"poly2tri.lib",
            @"polyclipping.lib"
        );
    }

    public override void ConfigureDebug(Configuration conf, EngineTarget target)
    {
        base.ConfigureDebug(conf, target);

        conf.LibraryFiles.Add
        (
            @"assimp-vc143-mtd.lib"
        );
    }

    public override void ConfigureRelease(Configuration conf, EngineTarget target)
    {
        base.ConfigureRelease(conf, target);

        conf.LibraryFiles.Add
        (
            @"assimp-vc143-mt.lib"
        );
    }
}