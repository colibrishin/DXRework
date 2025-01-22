using System;
using System.Collections;
using Microsoft.Win32;
using Sharpmake;

public abstract class ExportProject : Project
{
    protected ExportProject() : base(typeof(EngineTarget))
    {
        IsFileNameToLower = false;
        IsTargetFileNameToLower = false;

        AddTargets(new EngineTarget(
                ELaunchType.Editor | ELaunchType.Client | ELaunchType.Server,
                Platform.win64,
                DevEnv.vs2022,
                Optimization.Debug | Optimization.Release,
                OutputType.Lib,
                Blob.NoBlob,
                BuildSystem.FastBuild
        ));
    }

    [Configure()]
    public virtual void ConfigureAll(Configuration conf, EngineTarget target)
    {
    }

    [Configure(Optimization.Debug)] 
    public virtual void ConfigureDebug(Configuration conf, EngineTarget target)
    {
    }

    [Configure(Optimization.Release)]
    public virtual void ConfigureRelease(Configuration conf, EngineTarget target)
    {
    }
}

public class VCPKG : ExportProject
{
    protected VCPKG() 
    {
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target) 
    {
        base.ConfigureAll(conf, target);
    }

    public override void ConfigureRelease(Configuration conf, EngineTarget target)
    {
        base.ConfigureRelease(conf, target);
        string SolutionDir = Utils.GetSolutionDir();

        // Add root include path for vcpkg packages.
        conf.IncludePaths.Add(SolutionDir + @"\vcpkg_installed\x64-windows\include");

        // Add root lib path for vcpkg packages.
        conf.LibraryPaths.Add(SolutionDir + @"\vcpkg_installed\x64-windows\lib");
    }

    public override void ConfigureDebug(Configuration conf, EngineTarget target)
    {
        base.ConfigureDebug(conf, target);
        string SolutionDir = Utils.GetSolutionDir();

        // Add root include path for vcpkg packages.
        conf.IncludePaths.Add(SolutionDir + @"\vcpkg_installed\x64-windows\include");

        // Add root lib path for vcpkg packages.
        conf.LibraryPaths.Add(SolutionDir + @"\vcpkg_installed\x64-windows\debug\lib");
    }
}