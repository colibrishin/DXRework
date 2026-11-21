using System;
using System.Collections;
using System.IO;
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

    // assimp names its binaries after the MSVC toolset it detects at build time (e.g. assimp-vc143-mt or
    // assimp-vc145-mt), which depends on the compiler that built the vcpkg package. Use whichever is installed.
    private string FindAssimpName(EngineTarget target, string suffix)
    {
        string libDir = Utils.GetSolutionDir() + (target.Optimization == Optimization.Debug
            ? @"/vcpkg_installed/x64-windows/debug/lib"
            : @"/vcpkg_installed/x64-windows/lib");
        string[] found = Directory.Exists(libDir)
            ? Directory.GetFiles(libDir, "assimp-vc*-" + suffix + ".lib")
            : new string[0];
        if (found.Length != 1)
        {
            throw new Error("Expected one assimp-vc*-" + suffix + ".lib in " + libDir + ", found " + found.Length);
        }
        return Path.GetFileNameWithoutExtension(found[0]);
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target) 
    {
        base.ConfigureAll(conf, target);

        conf.LibraryFiles.Add
        (
            @"poly2tri.lib",
            @"polyclipping.lib",
            @"minizip.lib",
            @"draco.lib",
            @"polyclipping.lib",
            @"pugixml.lib"
        );

        conf.AddPublicDependency<Pugixml>(target);
    }

    public override void ConfigureDebug(Configuration conf, EngineTarget target)
    {
        base.ConfigureDebug(conf, target);
        string assimp = FindAssimpName(target, "mtd");

        conf.LibraryFiles.Add
        (
            assimp + ".lib",
            @"zlibd.lib"
        );

        string BinPath = GetVCPKGBinPath(target);
        conf.TargetCopyFiles.Add
        (
            BinPath + "/" + assimp + ".dll",
            BinPath + @"/poly2tri.dll",
            BinPath + @"/zlibd1.dll",
            BinPath + @"/kubazip.dll",
            BinPath + @"/minizip.dll",
            BinPath + @"/draco.dll"
        );
    }

    public override void ConfigureRelease(Configuration conf, EngineTarget target)
    {
        base.ConfigureRelease(conf, target);
        string assimp = FindAssimpName(target, "mt");

        conf.LibraryFiles.Add
        (
            assimp + ".lib",
            @"zlib.lib"
        );

        string BinPath = GetVCPKGBinPath(target);

        conf.TargetCopyFiles.Add
        (
            BinPath + "/" + assimp + ".dll",
            BinPath + @"/poly2tri.dll",
            BinPath + @"/zlib1.dll",
            BinPath + @"/minizip.dll",
            BinPath + @"/kubazip.dll",
            BinPath + @"/draco.dll"
        );
    }
}