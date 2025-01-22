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
            @"polyclipping.lib",
            @"minizip.lib",
            @"draco.lib"
        );

        conf.AddPublicDependency<Pugixml>(target);
    }

    public override void ConfigureDebug(Configuration conf, EngineTarget target)
    {
        base.ConfigureDebug(conf, target);

        conf.LibraryFiles.Add
        (
            @"assimp-vc143-mtd.lib",
            @"zlibd.lib"
        );

        string BinPath = GetVCPKGBinPath(target);
        conf.TargetCopyFiles.Add
        (
            BinPath + @"/assimp-vc143-mtd.dll",
            BinPath + @"/poly2tri.dll",
            BinPath + @"/zlibd1.dll",
            BinPath + @"/minizip.dll",
            BinPath + @"/draco.dll"
        );
    }

    public override void ConfigureRelease(Configuration conf, EngineTarget target)
    {
        base.ConfigureRelease(conf, target);

        conf.LibraryFiles.Add
        (
            @"assimp-vc143-mt.lib",
            @"zlib.lib"
        );

        string BinPath = GetVCPKGBinPath(target);

        conf.TargetCopyFiles.Add
        (
            BinPath + @"/assimp-vc143-mt.dll",
            BinPath + @"/poly2tri.dll",
            BinPath + @"/zlib1.dll",
            BinPath + @"/minizip.dll",
            BinPath + @"/draco.dll"
        );
    }
}